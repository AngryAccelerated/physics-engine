#include "alrenderer.h"



alRenderer::alRenderer(const bool visible, const QColor &centerColor)
    : QObject(), m_visible(visible), m_angleLineThickness(1), m_centerColor(centerColor)
{
    m_strokePen = QPen(Qt::darkCyan, m_thickness, Qt::SolidLine, Qt::RoundCap);
    QColor bc(Qt::darkGreen);
    bc.setAlphaF(0.12);
    m_fillBrush = QBrush(bc);
}


void alRenderer::handleMousePressEvent(QMouseEvent *e)
{

}

void alRenderer::handleMouseMoveEvent(QMouseEvent *e)
{

}

void alRenderer::handleMouseReleaseEvent(QMouseEvent *e)
{

}

void alRenderer::handlePaintEvent(QPainter *e)
{

}

alMeasurer::alMeasurer(const qreal arrowSize, const qreal fontSize, const qreal distance, const qreal thickness, const QColor& accelerationColor, const QColor& velocityColor):
    alRenderer(), m_accelerationColor(accelerationColor), m_velocityColor(velocityColor), m_arrowSize(arrowSize), m_fontSize(fontSize), m_textDistance(distance)
{
    m_type = RenderType::Measurer;
    m_thickness = thickness;
    m_visible = true;
    m_centerPathVisible = false;
}

void alMeasurer::renderArrow(QPainter * painter,const QPointF& start, const QPointF& end, const QString& text, const QColor& color)
{
    painter->setPen(QPen(color, m_thickness, Qt::SolidLine, Qt::RoundCap));
    QLineF arrowLine(start, end);
    qreal dist = arrowLine.length();
    painter->drawLine(arrowLine);
    //draw arrow head
    QPolygonF head;

    double angle = std::atan2(-arrowLine.dy(), arrowLine.dx());

    QPointF arrowP1 = arrowLine.p2() - QPointF(sin(angle + M_PI / 3) * m_arrowSize,
                                               cos(angle + M_PI / 3) * m_arrowSize);
    QPointF arrowP2 = arrowLine.p2() - QPointF(sin(angle + M_PI - M_PI / 3) * m_arrowSize,
                                               cos(angle + M_PI - M_PI / 3) * m_arrowSize);

    head.clear();
    head << arrowLine.p2() << arrowP1 << arrowP2;

    QPainterPath path;
    path.addPolygon(head);
    painter->fillPath(path,QBrush(color));
    //draw text

    qreal angleDegree = -angle * 180 / M_PI;
    double textRadiusAngle = M_PI/2 - angle;
    painter->setFont(QFont("Consolas", m_fontSize));
    if(angleDegree >= 90 || angleDegree <= -90)//2 and 3 quadrant
    {
        QPointF ph1 = (arrowLine.p2() + QPointF(cos(textRadiusAngle),sin(textRadiusAngle)) * m_textDistance);
        QPointF ph2 = (arrowLine.p1() + QPointF(cos(textRadiusAngle),sin(textRadiusAngle)) * m_textDistance);
        QLineF lf1(ph1, ph2);
        painter->translate(ph1);
        qreal arc = std::atan2(-lf1.dy(),lf1.dx());
        qreal degree = arc * 180 / M_PI;
        painter->rotate(-degree);
        painter->drawText(QPointF(-m_textDistance + dist / 2, 3 * m_textDistance),text);
        painter->rotate(degree);
        painter->translate(-ph1);
    }
    else
    {
        QPointF f(arrowLine.p1() + QPointF(cos(textRadiusAngle),sin(textRadiusAngle)) * m_textDistance);
        painter->translate(f);
        painter->rotate(angleDegree);
        painter->drawText(QPointF(dist / 2 - m_textDistance, - 2 *m_textDistance),text);
        painter->rotate(-angleDegree);
        painter->translate(-f);
    }
}

void alMeasurer::render(QPainter *e)
{
    if(m_visible)
    {
        foreach (alBody * m, m_bodyList) {
            QPointF mass(m->position().x(),m->position().y());
            QPointF velocity = mass + QPointF(m->velocity().x(),m->velocity().y()) * alLengthForceRatio;
            QPointF acceleration = mass + QPointF(m->acceleration().x(),m->acceleration().y()) * alLengthForceRatio * alLengthForceRatio;
            if(m->velocity().lengthSquare() > 0)
                renderArrow(e, mass, velocity, "v = " + QString::number(static_cast<int>(m->velocity().length())) + " m/s", m_velocityColor);
            if(m->acceleration().lengthSquare() > 0)
                renderArrow(e, mass, acceleration, "a = " + QString::number(static_cast<int>(m->acceleration().length())) + " m/s2", m_accelerationColor);
        }
    }
}

alCircleRenderer::alCircleRenderer(const qreal angleLineThickness):
    alBodyRenderer()
{
    m_angleLineThickness = angleLineThickness;
    m_type = RenderType::Circle;
}
void alCircleRenderer::render(QPainter *e)
{
    if(m_visible)
    {
        foreach (alCircle* circle, m_circleList) {
            renderPositionCenter(e, circle, m_strokePen.color());
            //draw circle body
            m_touchPen.setWidth(m_thickness);
            m_strokePen.setWidth(m_thickness);
            e->setPen(static_cast<alBody *>(circle)->isTouched() ? m_touchPen : m_strokePen);
            QRectF r(circle->position().x() - circle->radius(),circle->position().y() - circle->radius(),circle->radius() * 2,circle->radius() * 2);
            e->drawEllipse(r);
            //draw angle line
            m_strokePen.setWidth(m_angleLineThickness);
            e->setPen(m_strokePen);
            alVecter2 v = alRotation(circle->angle()) * alVecter2(circle->radius(), 0);
            QLineF l = QLineF(circle->position().x(), circle->position().y(),circle->position().x() + v.x(), circle->position().y() + v.y());
            e->drawLine(l);
            //fill circle
            QPainterPath p;
            p.addEllipse(r);
            e->fillPath(p,m_fillBrush);
        }
    }
}


alRectangleRenderer::alRectangleRenderer(const qreal borderThickness):
    alPolygonRenderer()
{
    m_thickness = borderThickness;
    m_type = RenderType::Polygon;
}

void alRectangleRenderer::render(QPainter *e)
{
    if(m_visible)
    {
        foreach (alRectangle* rectangle, m_rectangleList) {
            m_rectVertex = updateVertices(rectangle);
            renderPositionCenter(e, rectangle, m_strokePen.color());
            //render angle line
            alVecter2 v = alRotation(rectangle->angle()) * alVecter2(rectangle->width() / 2, 0);
            QLineF l = QLineF(rectangle->position().x(), rectangle->position().y(),rectangle->position().x() + v.x(), rectangle->position().y() + v.y());
            m_strokePen.setWidth(m_angleLineThickness);
            e->setPen(m_strokePen);
            e->drawLine(l);
            m_touchPen.setWidth(m_thickness);
            m_strokePen.setWidth(m_thickness);
            e->setPen(static_cast<alBody *>(rectangle)->isTouched() ? m_touchPen : m_strokePen);
            e->drawPolygon(m_rectVertex);
            QPainterPath p;
            p.addPolygon(m_rectVertex);
            e->fillPath(p,m_fillBrush);
        }
    }
}
void alPolygonRenderer::render(QPainter *e)
{
    if(m_visible)
    {
        foreach (alPolygon* polygon, m_polygonList) {
            renderPositionCenter(e, polygon, m_strokePen.color());
            m_strokePen.setWidth(m_angleLineThickness);
            QPolygonF vertex = updateVertices(polygon);
            e->setPen(m_strokePen);
            e->drawLine(QLineF(polygon->position().x(), polygon->position().y(), vertex[0].x(),vertex[0].y()));
            m_strokePen.setWidth(m_thickness);
            m_touchPen.setWidth(m_thickness);
            e->setPen(static_cast<alBody *>(polygon)->isTouched() ? m_touchPen : m_strokePen);
            e->drawPolygon(vertex);
            QPainterPath p;
            p.addPolygon(vertex);
            e->fillPath(p,m_fillBrush);
        }
    }
}
void alWallRenderer::render(QPainter *e)
{
    if(m_visible)
    {
        foreach (alWall* w, m_wallList) {
            QPolygonF pw = updateVertices(w);
            e->setPen(m_strokePen);
            e->drawPolygon(pw);
            QPainterPath p;
            p.addPolygon(pw);
            e->fillPath(p,m_fillBrush);
        }
    }
}

void alBodyRenderer::renderPositionCenter(QPainter *e, alBody *body, const QColor &color)
{
    e->setPen(QPen(color, 6, Qt::SolidLine, Qt::RoundCap));
    e->drawPoint(QPointF(body->position().x(), body->position().y()));
}

void alBodyRenderer::renderMassCenter(QPainter *e, alBody *body, const QColor &color)
{
    e->setPen(QPen(color, 6, Qt::SolidLine, Qt::RoundCap));
    e->drawPoint(QPointF(body->position().x(), body->position().y()));
}

