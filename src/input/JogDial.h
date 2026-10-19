#pragma once

#include <QPointF>
#include <QtMath>

// Turns stick rotation into discrete steps, like the jog wheel on a VCR
// remote. Push the stick to its rim and roll it around: every `stepDegrees`
// of travel produces one step, positive for clockwise.
//
// Rotation only counts while the stick is near its rim, so ordinary
// flicks and pushes in one direction never produce steps.
class JogDial
{
public:
    void setStepDegrees(qreal degrees) { m_step = qMax<qreal>(1.0, degrees); }
    qreal stepDegrees() const { return m_step; }

    // Feed the current (deadzone-corrected) stick position each poll.
    // Returns the number of steps crossed since the last call.
    int update(const QPointF &stick)
    {
        const qreal mag = qSqrt(stick.x() * stick.x() + stick.y() * stick.y());
        if (mag < (m_engaged ? kReleaseMag : kEngageMag)) {
            m_engaged = false;
            m_accum = 0;
            return 0;
        }
        // SDL's y axis points down, so increasing atan2 is clockwise on screen.
        const qreal angle = qRadiansToDegrees(qAtan2(stick.y(), stick.x()));
        if (!m_engaged) {
            m_engaged = true;
            m_last = angle;
            return 0;
        }
        qreal delta = angle - m_last;
        if (delta > 180)
            delta -= 360;
        else if (delta <= -180)
            delta += 360;
        m_last = angle;
        // A jump this large between polls is the stick snapping across the
        // center, not a rotation.
        if (qAbs(delta) > kMaxDeltaPerPoll)
            return 0;

        m_accum += delta;
        int steps = 0;
        while (m_accum >= m_step) {
            m_accum -= m_step;
            ++steps;
        }
        while (m_accum <= -m_step) {
            m_accum += m_step;
            --steps;
        }
        return steps;
    }

private:
    static constexpr qreal kEngageMag = 0.75;
    static constexpr qreal kReleaseMag = 0.55;
    static constexpr qreal kMaxDeltaPerPoll = 90;

    qreal m_step = 20;
    qreal m_last = 0;
    qreal m_accum = 0;
    bool m_engaged = false;
};
