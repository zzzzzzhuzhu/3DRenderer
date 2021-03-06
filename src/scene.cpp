#include "../include/scene.h"


/***
 *       _____                     
 *      / ____|                    
 *     | (___   ___ ___ _ __   ___ 
 *      \___ \ / __/ _ \ '_ \ / _ \
 *      ____) | (_|  __/ | | |  __/
 *     |_____/ \___\___|_| |_|\___|
 *                                 
 *                                 
 */

Scene::Scene(unsigned int refreshRate, QString envFile, std::vector<QString> vehList) : 
    m_camera(0.0f, 0.0f,QVector3D(0.0f, 0.0f, 0.0f)),
    m_frame(QVector3D(0.0f, 0.0f, 1.0f)),
    m_timestep(0.0f),
    m_refreshRate(refreshRate),
    m_timeRate(1.0f),
    m_frameRate(1 / static_cast<float>(m_refreshRate)),
    m_loop(true),
    m_showGlobalFrame(false),
    m_envFile(envFile),
    m_vehList(vehList), 
    m_snapshotMode(false),
    m_numSnapshot(5),
    m_vehFollow(0) {}


Scene::~Scene() {}


void Scene::initialize() {
    // Set up the scene light
    QVector4D lightDirection(1.0f,-1.0f, -1.0f, 0.0f);
    QVector3D lightIntensity(1.0f, 1.0f, 1.0f);
    m_light = CasterLight(lightIntensity, lightDirection);

    // Set up the skybox
    m_skybox.initialize();
    m_frame.initialize();
    
    // Load the objects of the environment
    Loader loader;
    loader.parse(m_envFile);
    p_graph = loader.getSceneGraph();
    
    // Create the vehicle
    for (auto it = m_vehList.begin(); it != m_vehList.end(); it++) {
        VehicleBuilder vehicleBuilder(*it);
        if (vehicleBuilder.build()) {
            std::unique_ptr<Vehicle> vehicle = vehicleBuilder.getVehicle();
            m_vehicles.push_back(std::move(vehicle));
        }
    }

    // Initialize all the loaded objects
    ObjectManager::initialize();
    
    // Get the simulation duration from the vehicle trajectory
    m_firstTimestep = 0.0f;
    m_finalTimestep = 1.0f;
    for (unsigned int i = 0; i < m_vehicles.size(); i++) {
        if (m_vehicles.at(i) != nullptr) {
            m_firstTimestep = 
                std::min(m_firstTimestep, m_vehicles.at(i)->getFirstTimeStep());
            m_finalTimestep = 
                std::max(m_finalTimestep, m_vehicles.at(i)->getFinalTimeStep());
        }
    }
}


void Scene::resize(int w, int h) {
    m_camera.setAspectRatio(static_cast<float>(w)/h);
}


void Scene::update() {
    // Update vehicle position
    for (unsigned int i = 0; i < m_vehicles.size(); i++) {
        if (m_vehicles.at(i) != nullptr) {
            m_vehicles.at(i)->updatePosition(m_timestep);
        }
    }
    
    // Get the position of the vehicle to follow
    Position vehiclePosition;
    if (m_vehFollow < m_vehicles.size()) {
        if (m_vehicles.at(m_vehFollow) != nullptr) {
            vehiclePosition = m_vehicles.at(m_vehFollow)->getPosition(m_timestep);
        }
    }
    
    // Update camera
    m_camera.trackObject(vehiclePosition);
    m_camera.processKeyboard();
    m_camera.processMouseMovement();
    m_camera.updateAxes();
    
    // Update the camera view matrix
    m_view = m_camera.getViewMatrix();
    // Update the camera projection matrix
    m_projection = m_camera.getProjectionMatrix();
    
    // Compute view and projection matrices of the light source
    m_cascades = {-0.3f, -10.0f, -20.0f, -35.0f};
//     m_view = m_light.getViewMatrix();
//     m_projection = m_light.getProjectionMatrix(m_camera, m_cascades).at(2);
    m_lightSpace = m_light.getLightSpaceMatrix(m_camera, m_cascades);
}


void Scene::render() {
    // Compute the end of each cascade in the clip space
    std::vector<float> endCascadeClip;
    for (unsigned int i = 0; i < m_cascades.size() - 1; i++) {
        QVector4D v(0.0f, 0.0f, m_cascades[i+1], 1.0f);
        v = m_projection * v;
        endCascadeClip.push_back(v.z());
    }
    
    // Call the render method of object in the scene
    m_skybox.render(m_view, m_projection);
    if (p_graph != nullptr)
        p_graph->render(m_light, m_view, m_projection, m_lightSpace, m_cascades);
    for (unsigned int i = 0; i < m_vehicles.size(); i++) {
        if (m_vehicles.at(i) != nullptr) {
            if (m_snapshotMode) {
                for (unsigned int k = 0; k < m_numSnapshot; k++) {
                    float timestep = m_firstTimestep + static_cast<float>(k)/m_numSnapshot * 
                        (m_finalTimestep - m_firstTimestep);
                    m_vehicles.at(i)->updatePosition(timestep);
                    m_vehicles.at(i)->render(
                        m_light, m_view, m_projection, m_lightSpace, m_cascades
                    );
                }
            } else {
                m_vehicles.at(i)->render(
                    m_light, m_view, m_projection, m_lightSpace, m_cascades
                );
            }
        }
    }
    if (m_showGlobalFrame) {
        m_frame.setModelMatrix(QMatrix4x4());
        m_frame.render(m_light, m_view, m_projection, m_lightSpace, m_cascades);
    }
}


void Scene::renderShadow(unsigned int cascadeIdx) {
    // Render the shadow map
    if (p_graph != nullptr)
        p_graph->renderShadow(m_lightSpace.at(cascadeIdx));
    for (unsigned int i = 0; i < m_vehicles.size(); i++) {
        if (m_vehicles.at(i) != nullptr) {
            if (m_snapshotMode) {
                for (unsigned int k = 0; k < m_numSnapshot; k++) {
                    float timestep = m_firstTimestep + static_cast<float>(k)/m_numSnapshot * 
                        (m_finalTimestep - m_firstTimestep);
                    m_vehicles.at(i)->updatePosition(timestep);
                    m_vehicles.at(i)->renderShadow(m_lightSpace.at(cascadeIdx));
                }
            } else {
                m_vehicles.at(i)->renderShadow(m_lightSpace.at(cascadeIdx));
            }
        }
    }
}


void Scene::cleanUp() {
    m_skybox.cleanUp();
    m_frame.cleanup();
    ObjectManager::cleanUp();
    TextureManager::cleanUp();
}


void Scene::updateTimestep() {
    // Update the timestep
    m_timestep += m_frameRate * m_timeRate;

    // Restart the animation if necessary
    if (m_loop && m_timestep > m_finalTimestep)
        m_timestep = m_firstTimestep;

    // Make sure the timestep is between the min and max value
    m_timestep = std::max(
        m_firstTimestep, std::min(m_timestep, m_finalTimestep)
    );
}



/***
 *       _____                         
 *      / ____|                        
 *     | (___   ___ ___ _ __   ___     
 *      \___ \ / __/ _ \ '_ \ / _ \    
 *      ____) | (_|  __/ | | |  __/    
 *     |_____/ \___\___|_| |_|\___|    
 *      | |               | |          
 *      | | ___   __ _  __| | ___ _ __ 
 *      | |/ _ \ / _` |/ _` |/ _ \ '__|
 *      | | (_) | (_| | (_| |  __/ |   
 *      |_|\___/ \__,_|\__,_|\___|_|   
 *                                     
 *                                     
 */

#include <QtXml>
#include <QFile>
#include <QDebug>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

Scene::Loader::Loader() {}

Scene::Loader::~Loader() {}

std::unique_ptr<Scene::Node> Scene::Loader::getSceneGraph() {
    return std::move(p_rootNode);
}

void Scene::Loader::parse(QString const fileNameIn) {
    QString defaultFile(":/xml/defaultWorld");
    QString fileName = fileNameIn;
    if (fileNameIn.isEmpty()) {
        // Use default file without warning.
        fileName = defaultFile;
    }
    else if (!QFile::exists(fileNameIn)) {
        qWarning() << 
            "The file" << fileName << "does not exist. Use default file" <<
            defaultFile << "instead.";
            fileName = defaultFile;
    }
    
    QDomDocument domDoc;
    // Load XML file as raw data
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        // Error while loading file
        qWarning() << "Error while loading file" << fileName;
    }
    
    // Retrieve the XML schema
    QXmlSchema schema;
    QUrl schemaUrl = QUrl::fromLocalFile(":/xml/world.xsd");
    if (!schema.load(schemaUrl)) {
        qDebug() << "Cannot load XSD schema. The XML file will not be parsed.";
        return;
    }
    // The XSD resource file cannot be invalid
    if (!schema.isValid()) {
        qCritical() << "The  XML schema (.xsd) is invalid. The XML file will"
            "not be parsed.";
        return;
    }

    // Validate the user file
    QXmlSchemaValidator validator{schema};
    if (!validator.validate(&file, QUrl::fromLocalFile(file.fileName()))) {
        qCritical() << "The file" << fileName << "does not meet the XML "
        "schema definition. The XML will not be parsed.";
        return;
    }
    
    // Set data into the QDomDocument before processing
    file.reset();
    domDoc.setContent(&file);
    file.close();
    
    // Extract the root
    QDomElement world = domDoc.documentElement();
    
    // Process the world element
    QDomElement group = world.firstChildElement();
    p_rootNode = std::make_unique<Node>();
    processGroup(group, p_rootNode);
}

bool Scene::Loader::qStringToQVector3D(const QString & string, QVector3D & vec) {
    QStringList list = string.split(" ");
    if (list.size() != 3)
        return false;
    bool ok;
    float x = list[0].toFloat(&ok); if (!ok) return false;
    float y = list[1].toFloat(&ok); if (!ok) return false;
    float z = list[2].toFloat(&ok); if (!ok) return false;
    vec = QVector3D(x, y, z);
    return true;
}


bool Scene::Loader::qStringToQVector4D(const QString & string, QVector4D & vec) {
    QStringList list = string.split(" ");
    if (list.size() != 4)
        return false;
    bool ok;
    float x = list[0].toFloat(&ok); if (!ok) return false;
    float y = list[1].toFloat(&ok); if (!ok) return false;
    float z = list[2].toFloat(&ok); if (!ok) return false;
    float w = list[3].toFloat(&ok); if (!ok) return false;
    vec = QVector4D(x, y, z, w);
    return true;
}

void Scene::Loader::processGroup(
    const QDomElement & group, std::unique_ptr<Node> & node
) {
    // Make sure this is a group element
    if (group.tagName().compare("group") != 0)
        return;
    
    // Process children elements
    for (
        QDomElement elmt = group.firstChildElement(); 
        !elmt.isNull();
        elmt = elmt.nextSiblingElement()
    ) {
        // Process transform
        if (elmt.tagName().compare("transform") == 0) {
            QMatrix4x4 parentMatrix = node->getWorldMatrix();
            node->addChild(processTransform(elmt, parentMatrix));
        }
    }
}


std::unique_ptr<Scene::Node>  Scene::Loader::processTransform(
    const QDomElement & transform, const QMatrix4x4 parentMatrix
) {
    // Make sure this is a transform element
    if (transform.tagName().compare("transform") != 0)
        return std::unique_ptr<Scene::Node>();
    
    // Compute the local transformation matrix
    QString transString = transform.attribute("translation","0.0 0.0 0.0");
    QVector3D trans;
    if (!qStringToQVector3D(transString, trans))
        trans = QVector3D(0, 0, 0);
    
    QString rotString = transform.attribute("rotation", "0.0 0.0 0.0 0.0");
    QVector4D rot;
    if (!qStringToQVector4D(rotString, rot))
        rot = QVector4D(1, 0, 0, 0);
    
    QString scaleString = transform.attribute("scale", "1.0 1.0 1.0");
    QVector3D scale;
    if (!qStringToQVector3D(scaleString, scale))
        scale = QVector3D(1, 1, 1);
    
    QMatrix4x4 localMatrix;
    localMatrix.setToIdentity();
    localMatrix.translate(trans);
    localMatrix.rotate(QQuaternion(rot));
    localMatrix.scale(scale);
    
    // Create new node for transformed object
    std::unique_ptr<Node> node;
    node = std::make_unique<Node>(parentMatrix, localMatrix);
    
    // Process the object moved by the transformation
    for (
        QDomElement elmt = transform.firstChildElement(); 
        !elmt.isNull();
        elmt = elmt.nextSiblingElement()
    ) {
        if (elmt.tagName().compare("group") == 0) {
            processGroup(elmt, node);
        }
        else if (elmt.tagName().compare("model") == 0) {
            node->addObject(processModel(elmt));
        }
        else if (elmt.tagName().compare("shape") == 0) {
            node->addObject(processShape(elmt));
        }
        else if (elmt.tagName().compare("reference") == 0) {
            node->addObject(processReference(elmt));
        }
    }
    
    // Return transformed node
    return node;
}


ABCObject *  Scene::Loader::processModel(const QDomElement & elmt) {
    if (elmt.tagName().compare("model") != 0)
        return nullptr;
    
    // Retrieve name
    QString name = elmt.attribute("name","");
    if (name.isEmpty()) {
        qWarning() << 
            "The attribute 'name' of the element 'model' must be provided. "
            "The object will not be rendered.";
        return nullptr;
    }
    
    // Check that the ID of the plan is unique
    if (ObjectManager::getObject(name) != nullptr) {
        qWarning() << 
            "An object with name" << name << "already exists. The object will "
            "not be rendered.";
        return nullptr;
    }
    
    // Retrieve attributes
    QString fileName = elmt.attribute("url","");
    QString textureDir = elmt.attribute("textureFolder","");
    
    // Build object and add it to the container
    ABCObject * model = nullptr;
    Object::Loader modelLoader(fileName, textureDir);
    if (modelLoader.build()) {
        model = ObjectManager::loadObject(name, modelLoader.getObject());
        return model;
    }
    return nullptr;
}


ABCObject * Scene::Loader::processShape(const QDomElement & elmt) {
    if (elmt.tagName().compare("shape") != 0)
        return nullptr;
    
    // Retrieve name
    QString name = elmt.attribute("name","");
    if (name.isEmpty()) {
        qWarning() << 
            "The attribute 'name' of the element 'plane' must be provided. "
            "The object will not be rendered.";
        return nullptr;
    }
    
    // Check that the ID of the plan is unique
    if (ObjectManager::getObject(name) != nullptr) {
        qWarning() << 
            "An object with name" << name << "already exists. The object will "
            "not be rendered.";
        return nullptr;
    }
    
    // Build the object and add it to the container
    ABCObject * model = nullptr;
    Object::XmlLoader modelLoader(elmt);
    if (modelLoader.build()) {
        model = ObjectManager::loadObject(name, modelLoader.getObject());
        return model;
    }
    return nullptr;
}


ABCObject * Scene::Loader::processReference(const QDomElement& elmt) {
    if (elmt.tagName().compare("reference") != 0)
        return nullptr;
    
    QString ref = elmt.attribute("ref", "");
    ABCObject * object = ObjectManager::getObject(ref);
    return object;
}




/***
 *       _____                         
 *      / ____|                        
 *     | (___   ___ ___ _ __   ___     
 *      \___ \ / __/ _ \ '_ \ / _ \    
 *      ____) | (_|  __/ | | |  __/    
 *     |_____/_\___\___|_| |_|\___|    
 *        / ____|               | |    
 *       | |  __ _ __ __ _ _ __ | |__  
 *       | | |_ | '__/ _` | '_ \| '_ \ 
 *       | |__| | | | (_| | |_) | | | |
 *        \_____|_|  \__,_| .__/|_| |_|
 *                        | |          
 *                        |_|          
 */

void Scene::Node::render(
    const CasterLight & light, const QMatrix4x4 & view, 
    const QMatrix4x4 & projection, 
    const std::array<QMatrix4x4,NUM_CASCADES> & lightSpace,
    const std::array<float,NUM_CASCADES+1> & cascades
) {
    // Draw the node
    for (auto it = m_objects.begin(); it != m_objects.end(); it++) {
        if (*it != nullptr) {
            // Set model matrix
            (*it)->setModelMatrix(m_worldMatrix);
            
            // Draw
            (*it)->render(
                light, view, projection, lightSpace, cascades
            );
        }
    }
    
    // Draw its descendant
    for (auto it = m_children.begin(); it != m_children.end(); it++) {
        (*it)->render(light, view, projection, lightSpace, cascades);
    }
}


void Scene::Node::renderShadow(const QMatrix4x4& lightSpace) {
    // Draw the node
    for (auto it = m_objects.begin(); it != m_objects.end(); it++) {
        if (*it != nullptr) {
            // Set model matrix
            (*it)->setModelMatrix(m_worldMatrix);
            
            // Draw
            (*it)->renderShadow(lightSpace);
        }
    }
    
    // Draw its descendant
    for (auto it = m_children.begin(); it != m_children.end(); it++) {
        (*it)->renderShadow(lightSpace);
    }
}









