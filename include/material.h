#ifndef MATERIAL_H
#define MATERIAL_H

#include <QString>
#include <QVector3D>
#include "texture.h"

/// Material class
/**
 * @brief This class defines a material applied to an object.
 * @author Louis Filipozzi
 */
class Material {
public:
    /**
     * @brief Constructor of the material.
     * @remark If the texture is not provided, a default texture is set.
     */
    Material(QString name, Texture * texture = nullptr);
    ~Material() {};
    
    void setAmbientColor(QVector3D color) {m_ambient = color;};
    void setDiffuseColor(QVector3D color) {m_diffuse = color;};
    void setSpecularColor(QVector3D color) {m_specular = color;};
    void setShininess(float shininess) {m_shininess = shininess;};
    void setAlpha(float alpha) {m_alpha = alpha;};
    void setTexture(Texture * texture);
    
    QVector3D getAmbientColor() const {return m_ambient;};
    QVector3D getDiffuseColor() const {return m_diffuse;};
    QVector3D getSpecularColor() const {return m_specular;};
    float getShininess() const {return m_shininess;};
    float getAlpha() const {return m_alpha;};
    Texture * getTexture() const {return m_texture;};
    
private:
    /**
     * @brief set the default texture of the material.
     */
    void setDefaultTexture();
    
    QString m_name;
    QVector3D m_ambient;
    QVector3D m_diffuse;
    QVector3D m_specular;
    float m_shininess;
    float m_alpha;
    Texture * m_texture;
};

#endif // MATERIAL_H