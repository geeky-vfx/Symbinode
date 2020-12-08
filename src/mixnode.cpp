#include "mixnode.h"
#include <iostream>

MixNode::MixNode(QQuickItem *parent, QVector2D resolution, float factor, int mode):
    Node(parent, resolution), m_factor(factor), m_mode(mode)
{
    preview = new MixObject(grNode, m_resolution, m_factor, m_mode);
    float s = scaleView();
    preview->setTransformOrigin(TopLeft);
    preview->setWidth(174);
    preview->setHeight(174);
    preview->setX(3*s);
    preview->setY(30*s);
    preview->setScale(s);
    createSockets(4, 1);
    setTitle("Mix");
    m_socketsInput[0]->setTip("Color1");
    m_socketsInput[1]->setTip("Color2");
    m_socketsInput[2]->setTip("Factor");
    m_socketsInput[3]->setTip("Mask");
    connect(this, &MixNode::changeSelected, this, &MixNode::updatePrev);
    connect(this, &Node::changeScaleView, this, &MixNode::updateScale);
    connect(preview, &MixObject::textureChanged, this, &MixNode::setOutput);
    connect(this, &MixNode::generatePreview, this, &MixNode::previewGenerated);
    connect(preview, &MixObject::updatePreview, this, &MixNode::updatePreview);
    connect(this, &Node::changeResolution, preview, &MixObject::setResolution);
    propView = new QQuickView();
    propView->setSource(QUrl(QStringLiteral("qrc:/qml/MixProperty.qml")));
    propertiesPanel = qobject_cast<QQuickItem*>(propView->rootObject());
    connect(propertiesPanel, SIGNAL(factorChanged(qreal)), this, SLOT(updateFactor(qreal)));
    connect(propertiesPanel, SIGNAL(modeChanged(int)), this, SLOT(updateMode(int)));
    propertiesPanel->setProperty("startFactor", m_factor);
    propertiesPanel->setProperty("startMode", m_mode);
}

MixNode::~MixNode() {
    delete preview;
}

void MixNode::operation() {

    preview->setFirstTexture((m_socketsInput[0]->value().toUInt()));
    preview->setSecondTexture((m_socketsInput[1]->value().toUInt()));
    preview->setMaskTexture(m_socketsInput[3]->value().toUInt());

    if(m_socketsInput[2]->countEdge() > 0) {
        preview->useFactorTexture = true;
        preview->setFactor(m_socketsInput[2]->value());
    }
    else {
        preview->useFactorTexture = false;
        preview->setFactor(m_factor);
    }
    preview->setMode(m_mode);
    preview->mixedTex = true;
    preview->selectedItem = selected();
    preview->update();

    if(m_socketsInput[0]->countEdge() == 0 && m_socketsInput[1]->countEdge() == 0) m_socketOutput[0]->setValue(0);
}

float MixNode::factor() {
    return m_factor;
}

void MixNode::setFactor(float f) {
    m_factor = f;
    factorChanged(f);
}

int MixNode::mode() {
    return m_mode;
}

void MixNode::setMode(int mode) {
    m_mode = mode;
    modeChanged(mode);
}

void MixNode::setOutput() {
    m_socketOutput[0]->setValue(preview->texture());
}

void MixNode::serialize(QJsonObject &json) const {
    Node::serialize(json);
    json["type"] = 7;
    json["factor"] = m_factor;
    json["mode"] = m_mode;
}

void MixNode::deserialize(const QJsonObject &json) {
    Node::deserialize(json);
    if(json.contains("factor")) {
        setFactor(json["factor"].toVariant().toFloat());
        propertiesPanel->setProperty("startFactor", m_factor);
    }
    if(json.contains("mode")) {
        setMode(json["mode"].toInt());
        propertiesPanel->setProperty("startMode", m_mode);
    }
}

void MixNode::updateFactor(qreal f) {
    setFactor(static_cast<float>(f));
    if(m_socketsInput[2]->countEdge() == 0) {
        operation();
    }
    dataChanged();
}

void MixNode::updateMode(int mode) {
    setMode(mode);
    operation();
    dataChanged();
}

void MixNode::updateScale(float scale) {
    preview->setX(3*scale);
    preview->setY(30*scale);
    preview->setScale(scale);
}

void MixNode::updatePrev(bool sel) {
    if(sel) {
         updatePreview(preview->texture(), true);
    }
}

void MixNode::previewGenerated() {
    preview->mixedTex = true;
    preview->update();
}