#include "inverse.h"
#include <QOpenGLFramebufferObjectFormat>

InverseObject::InverseObject(QQuickItem *parent, QVector2D resolution):QQuickFramebufferObject (parent),
    m_resolution(resolution)
{
     setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer *InverseObject::createRenderer() const {
    return new InverseRenderer(m_resolution);
}

unsigned int &InverseObject::texture() {
    return m_texture;
}

void InverseObject::setTexture(unsigned int texture) {
    m_texture = texture;
    textureChanged();
}

unsigned int InverseObject::sourceTexture() {
    return m_sourceTexture;
}

void InverseObject::setSourceTexture(unsigned int texture) {
    m_sourceTexture = texture;
    inversedTex = true;
    update();
}

QVector2D InverseObject::resolution() {
    return m_resolution;
}

void InverseObject::setResolution(QVector2D res) {
    m_resolution = res;
    resUpdated = true;
    update();
}

InverseRenderer::InverseRenderer(QVector2D res): m_resolution(res) {
    initializeOpenGLFunctions();
    inverseShader = new QOpenGLShaderProgram();
    inverseShader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texture.vert");
    inverseShader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/inverse.frag");
    inverseShader->link();
    textureShader = new QOpenGLShaderProgram();
    textureShader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texture.vert");
    textureShader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/texture.frag");
    textureShader->link();
    inverseShader->bind();
    inverseShader->setUniformValue(inverseShader->uniformLocation("sourceTexture"), 0);
    inverseShader->release();
    textureShader->bind();
    textureShader->setUniformValue(textureShader->uniformLocation("textureSample"), 0);
    textureShader->release();
    float vertQuadTex[] = {-1.0f, -1.0f, 0.0f, 0.0f,
                    -1.0f, 1.0f, 0.0f, 1.0f,
                    1.0f, -1.0f, 1.0f, 0.0f,
                    1.0f, 1.0f, 1.0f, 1.0f};
    unsigned int VBO;
    glGenVertexArrays(1, &textureVAO);
    glBindVertexArray(textureVAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertQuadTex), vertQuadTex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glGenFramebuffers(1, &inverseFBO);
    glGenTextures(1, &m_inversedTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, inverseFBO);
    glBindTexture(GL_TEXTURE_2D, m_inversedTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, m_resolution.x(), m_resolution.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_inversedTexture, 0
    );
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

InverseRenderer::~InverseRenderer() {
    delete inverseShader;
    delete textureShader;
}

QOpenGLFramebufferObject *InverseRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(8);
    return new QOpenGLFramebufferObject(size, format);
}

void InverseRenderer::synchronize(QQuickFramebufferObject *item) {
    InverseObject *inverseItem = static_cast<InverseObject*>(item);
    if(inverseItem->resUpdated) {
        inverseItem->resUpdated = false;
        m_resolution = inverseItem->resolution();
        updateTexResolution();
    }
    if(inverseItem->inversedTex) {
        inverseItem->inversedTex = false;
        m_sourceTexture = inverseItem->sourceTexture();
        if(m_sourceTexture) {
            inverte();
            inverseItem->setTexture(m_inversedTexture);
            if(inverseItem->selectedItem) {
                inverseItem->updatePreview(m_inversedTexture, true);
            }
        }
    }    
}

void InverseRenderer::render() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ZERO, GL_ONE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(m_sourceTexture) {
        glBindVertexArray(textureVAO);
        textureShader->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_inversedTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        textureShader->release();
        glBindVertexArray(0);
    }
}

void InverseRenderer::inverte() {
    glBindFramebuffer(GL_FRAMEBUFFER, inverseFBO);
    glViewport(0, 0, m_resolution.x(), m_resolution.y());
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(textureVAO);
    inverseShader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sourceTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    inverseShader->release();
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void InverseRenderer::updateTexResolution() {
    glBindTexture(GL_TEXTURE_2D, m_inversedTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, m_resolution.x(), m_resolution.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}