#ifndef ABSTRACTOBJECT_H
#define ABSTRACTOBJECT_H

#include "light.h"
#include <QMatrix4x4>
#include <memory>

#include "constants.h"

/// Abstract object class
/**
 * @brief Define the abstract base class of a 3D object.
 * @author Louis Filipozzi
 */
class ABCObject {
public:
    virtual ~ABCObject() {};
    
    /**
     * @brief Initialize the object, i.e. create the buffers, attributes, ...
     */
    virtual void initialize() = 0;
    
    /**
     * @brief Draw the object.
     * @param view The view matrix.
     * @param projection The projection matrix.
     * @param lightSpace The view and projection matrices of the light (used for 
     * shadow mapping).
     * @param cascades Array containing the distance for cascade shadow mapping.
     */
    virtual void render(
        const CasterLight & light, const QMatrix4x4 & view, 
        const QMatrix4x4 & projection, 
        const std::array<QMatrix4x4,NUM_CASCADES> & lightSpace,
        const std::array<float,NUM_CASCADES+1> & cascades
    ) = 0;
    
    /**
     * @brief Draw the object when computing the framebuffer for shadow mapping.
     * @param lightSpace The view and projection matrix of the light (used for 
     * shadow mapping).
     */
    virtual void renderShadow(const QMatrix4x4 & lightSpace) = 0;
    
    /**
     * @brief Clean up the object.
     */
    virtual void cleanUp() = 0;
    
    /**
     * @brief Set the model matrix of the object to position the object as 
     * desired.
     * @param model The model matrix.
     */
    void setModelMatrix(const QMatrix4x4 & model) {m_model = model;};
    
protected:
    /**
     * Model matrix of the object.
     */
    QMatrix4x4 m_model;
};



/// Object manager
/**
 * @brief Manager of all ABCObject.
 * @author Louis Filipozzi
 */
class ObjectManager {
public:
    /**
     * @brief Load an Object and return a raw pointer to the object.
     * @param name The name of the object.
     * @param object The object to load.
     * @return A pointer to the object.
     */
    static ABCObject * loadObject(
        QString name, std::unique_ptr<ABCObject> object
    );
    
    /**
     * @brief Get the object.
     * @remark Return a null pointer if the model has not been loaded yet.
     * @param name The name of the model to load.
     * @return A pointer to the object.
     */
    static ABCObject * getObject(QString name);
    
    /**
     * @brief Initialize all the objects.
     */
    static void initialize();
    
    /**
     * @brief Properly deallocate all objects.
     */
    static void cleanUp();
    
private:
    ObjectManager() {};
    
    typedef std::map<QString, std::unique_ptr<ABCObject>> ObjectsMap;
    /**
     * List of loaded model.
     */
    static ObjectsMap m_objects;
};

#endif // ABSTRACTOBJECT_H
