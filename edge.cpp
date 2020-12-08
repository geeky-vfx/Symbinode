#include "edge.h"
#include "scene.h"
#include <iostream>
#include <QQmlEngine>
#include <QQmlProperty>

Edge::Edge(QQuickItem *parent): QQuickItem(parent)
{
    grEdge = new CubicBezier(this);
    if(parent) {
        Scene *scene = reinterpret_cast<Scene*>(parent);
        connect(scene->background(), &BackgroundObject::scaleChanged, this, &Edge::updateScale);
        grEdge->setLineWidth(3.0f*scene->background()->viewScale());
    }
}

Edge::Edge(const Edge &edge):Edge() {
    setStartPosition(edge.m_startPos);
    setEndPosition(edge.m_endPos);
}

Edge::~Edge() {
    m_startSocket = nullptr;
    m_endSocket = nullptr;
    delete grEdge;
}

QVector2D Edge::startPosition() {
    return m_startPos;
}

void Edge::setStartPosition(QVector2D pos) {
    m_startPos = pos;
    setX(std::min(m_startPos.x(), m_endPos.x()));
    setY(std::min(m_startPos.y(), m_endPos.y()));
    float w = std::abs(m_startPos.x() - m_endPos.x());
    float h = std::abs(m_startPos.y() - m_endPos.y());
    grEdge->setWidth(std::max(w, 0.01f));
    grEdge->setHeight(std::max(h, 0.01f));
    float p1X = w > 0.0f ? (pos.x() - x())/w : 0.0f;
    float p1Y = h > 0.0f ? (pos.y() - y())/h : 0.0f;
    float offset = 0.5f;
    grEdge->setP1(QPointF(p1X, p1Y));
    grEdge->setP2(QPointF(p1X + offset, p1Y));
    grEdge->setP4(QPointF(1.0f - p1X, 1.0f - p1Y));
    grEdge->setP3(QPointF(1.0f - offset - p1X, 1.0f - p1Y));
    emit startPositionChanged(pos);

}

QVector2D Edge::endPosition() {
    return m_endPos;
}

void Edge::setEndPosition(QVector2D pos) {
    m_endPos = pos;
    setX(std::min(m_startPos.x(), m_endPos.x()));
    setY(std::min(m_startPos.y(), m_endPos.y()));
    float w = std::abs(m_startPos.x() - m_endPos.x());
    float h = std::abs(m_startPos.y() - m_endPos.y());
    grEdge->setWidth(std::max(w, 0.01f));
    grEdge->setHeight(std::max(h, 0.01f));
    float p4X = w > 0.0f ? (pos.x() - x())/w : 0.0f;
    float p4Y = h > 0.0f ? (pos.y() - y())/h : 0.0f;
    float offset = 0.5f;
    grEdge->setP4(QPointF(p4X, p4Y));
    grEdge->setP3(QPointF(p4X - offset, p4Y));
    grEdge->setP1(QPointF(1.0f - p4X, 1.0f - p4Y));
    grEdge->setP2(QPointF(1.0f + offset - p4X, 1.0f - p4Y));
    emit endPositionChanged(pos);
}

Socket *Edge::startSocket() {
    return  m_startSocket;
}

void Edge::setStartSocket(Socket *socket) {
    m_startSocket = socket;
}

Socket *Edge::endSocket() {
    return m_endSocket;
}

void Edge::setEndSocket(Socket *socket) {
    if(m_endSocket) m_endSocket->reset();
    m_endSocket = socket;
    socket->setValue(m_startSocket->value());
}

Socket* Edge::findSockets(Scene *scene, float x, float y) {
    QQuickItem *item = scene->childAt(x, y);
    if(qobject_cast<Node*>(item)) {
        Node *n = qobject_cast<Node*>(item);
        QPointF childPos = n->mapFromItem(scene, QPointF(x, y));
        QQuickItem *child = n->childAt(childPos.x(), childPos.y());
        if(qobject_cast<Socket*>(child)) {
            Socket *s = qobject_cast<Socket*>(child);
            return s;
        }
    }
    return nullptr;
}

bool Edge::selected() {
    return m_selected;
}

void Edge::setSelected(bool selected) {
    m_selected = selected;
    grEdge->setProperty("selected", m_selected);
}

void Edge::serialize(QJsonObject &json) const {
    Scene *scene = reinterpret_cast<Scene*>(parentItem());
    json["name"] = objectName();
    json["startPosX"] = (m_startPos.x() + scene->background()->viewPan().x())/scene->background()->viewScale();
    json["startPosY"] = (m_startPos.y() + scene->background()->viewPan().y())/scene->background()->viewScale();
    json["endPosX"] = (m_endPos.x() + scene->background()->viewPan().x())/scene->background()->viewScale();
    json["endPosY"] = (m_endPos.y() + scene->background()->viewPan().y())/scene->background()->viewScale();
}

void Edge::deserialize(const QJsonObject &json) {
    Scene *scene = reinterpret_cast<Scene*>(parentItem());
    if(json.contains("startPosX") && json.contains("startPosY")) {
        setStartPosition(QVector2D(json["startPosX"].toVariant().toFloat(), json["startPosY"].toVariant().toFloat()));
        Socket *s = findSockets(scene, m_startPos.x(), m_startPos.y());
        if(s) {
            setStartSocket(s);
            s->addEdge(this);
        }
    }
    if(json.contains("endPosX") && json.contains("endPosY")) {
        setEndPosition(QVector2D(json["endPosX"].toVariant().toFloat(), json["endPosY"].toVariant().toFloat()));
        Socket *s = findSockets(scene, m_endPos.x(), m_endPos.y());
        if(s) {
            setEndSocket(s);
            s->addEdge(this);
        }
    }
}

void Edge::updateScale(float scale) {
    m_scale = scale;
    grEdge->setLineWidth(std::max(3.0f*scale, 1.0f));
}

void Edge::pressedEdge(bool control) {
    Scene *scene = reinterpret_cast<Scene*>(parentItem());
    if(control) {
        setSelected(!m_selected);
        if(m_selected) {
            QList<QQuickItem*> selected = scene->selectedList();
            scene->addSelected(this);
            scene->selectedItems(selected);
        }
        else {
            QList<QQuickItem*> selected = scene->selectedList();
            scene->deleteSelected(this);
            scene->selectedItems(selected);
        }

    }
    else {
        setSelected(true);
        QList<QQuickItem*> selected = scene->selectedList();
        scene->clearSelected();        
        scene->addSelected(this);
        scene->selectedItems(selected);
    }
}
