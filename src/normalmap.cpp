#include "normalmap.h"
#include <QOpenGLFramebufferObjectFormat>
#include <iostream>

NormalMapObject::NormalMapObject(QQuickItem *parent, QVector2D resolution, float strenght):
    QQuickFramebufferObject (parent), m_resolution(resolution), m_strenght(strenght)
{

}

QQuickFramebufferObject::Renderer *NormalMapObject::createRenderer() const {
    return new NormalMapRenderer(m_resolution);
}

unsigned int NormalMapObject::grayscaleTexture() {
    return m_grayscaleTexture;
}

void NormalMapObject::setGrayscaleTexture(unsigned int texture) {
    m_grayscaleTexture = texture;
    normalGenerated = true;
    update();
}

float NormalMapObject::strenght() {
    return m_strenght;
}

void NormalMapObject::setStrenght(float strenght) {
    m_strenght = strenght;
    normalGenerated = true;
    update();
}

QVector2D NormalMapObject::resolution() {
    return m_resolution;
}

void NormalMapObject::setResolution(QVector2D res) {
    m_resolution = res;
    resUpdated = true;
    update();
}

unsigned int NormalMapObject::normalTexture() {
    return m_normalTexture;
}

void NormalMapObject::setNormalTexture(unsigned int texture) {
    m_normalTexture = texture;
    textureChanged();
}

NormalMapRenderer::NormalMapRenderer(QVector2D resolution): m_resolution(resolution) {
    initializeOpenGLFunctions();

    normalMap = new QOpenGLShaderProgram();
    normalMap->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/noise.vert");
    normalMap->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/normalmap.frag");
    normalMap->link();

    textureShader = new QOpenGLShaderProgram();
    textureShader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texture.vert");
    textureShader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/texture.frag");
    textureShader->link();

    normalMap->bind();
    normalMap->setUniformValue(normalMap->uniformLocation("grayscaleTexture"), 0);
    normalMap->release();

    textureShader->bind();
    textureShader->setUniformValue(textureShader->uniformLocation("textureSample"), 0);
    textureShader->release();

    float vertQuad[] = {-1.0f, -1.0f,
                    -1.0f, 1.0f,
                    1.0f, -1.0f,
                    1.0f, 1.0f};
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertQuad), vertQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float vertQuadTex[] = {-1.0f, -1.0f, 0.0f, 1.0f,
                    -1.0f, 1.0f, 0.0f, 0.0f,
                    1.0f, -1.0f, 1.0f, 1.0f,
                    1.0f, 1.0f, 1.0f, 0.0f};
    unsigned int VBO2;
    glGenVertexArrays(1, &textureVAO);
    glBindVertexArray(textureVAO);
    glGenBuffers(1, &VBO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertQuadTex), vertQuadTex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenFramebuffers(1, &normalMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, normalMapFBO);
    glGenTextures(1, &m_normalTexture);
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_resolution.x(), m_resolution.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_normalTexture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

NormalMapRenderer::~NormalMapRenderer() {
    delete textureShader;
    delete normalMap;
}

QOpenGLFramebufferObject *NormalMapRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(8);
    return new QOpenGLFramebufferObject(size, format);
}

void NormalMapRenderer::synchronize(QQuickFramebufferObject *item) {
    NormalMapObject *normalItem = static_cast<NormalMapObject*>(item);    
    if(normalItem->resUpdated) {
        normalItem->resUpdated = false;
        m_resolution = normalItem->resolution();
        updateTexResolution();
    }    
    if(normalItem->normalGenerated) {
        normalItem->normalGenerated = false;
        m_grayscaleTexture = normalItem->grayscaleTexture();
        if(m_grayscaleTexture) {
            strenght = normalItem->strenght();
            createNormalMap();
            normalItem->setNormalTexture(m_normalTexture);
            if(normalItem->selectedItem) {
                normalItem->updatePreview(m_normalTexture, true);
            }
        }
    }
}

void NormalMapRenderer::render() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(m_grayscaleTexture) {
        textureShader->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_normalTexture);
        glBindVertexArray(textureVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        textureShader->release();
    }
}

void NormalMapRenderer::createNormalMap() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, normalMapFBO);
    glViewport(0, 0, m_resolution.x(), m_resolution.y());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    normalMap->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_grayscaleTexture);
    normalMap->setUniformValue(normalMap->uniformLocation("strength"), strenght);
    normalMap->setUniformValue(normalMap->uniformLocation("res"), m_resolution);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    normalMap->release();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void NormalMapRenderer::updateTexResolution() {
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_resolution.x(), m_resolution.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}