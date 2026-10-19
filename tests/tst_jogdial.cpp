#include <QtTest>

#include "input/JogDial.h"

namespace {
// Stick at the rim, `deg` degrees clockwise from pointing right
// (SDL y points down, so clockwise means increasing angle).
QPointF rim(qreal deg, qreal mag = 1.0)
{
    const qreal r = qDegreesToRadians(deg);
    return {mag * qCos(r), mag * qSin(r)};
}

// Rolls the stick from `from` to `to` degrees in `increment` slices,
// returning the total steps produced. Tests stop a few degrees past a step
// boundary: landing exactly on one is at the mercy of float rounding.
int roll(JogDial &jog, qreal from, qreal to, qreal increment = 5)
{
    int steps = 0;
    const qreal dir = to >= from ? 1 : -1;
    for (qreal a = from; dir * (to - a) >= 0; a += dir * increment)
        steps += jog.update(rim(a));
    return steps;
}
} // namespace

class TestJogDial : public QObject
{
    Q_OBJECT

private slots:
    void clockwiseStepsEveryTwentyDegrees()
    {
        JogDial jog;
        QCOMPARE(roll(jog, 0, 19), 0);  // first touch engages, no step yet
        QCOMPARE(roll(jog, 20, 45), 2);
        QCOMPARE(roll(jog, 50, 365), 16); // a full turn is 18 steps in total
    }

    void counterClockwiseIsNegative()
    {
        JogDial jog;
        QCOMPARE(roll(jog, 0, -65), -3);
    }

    void reversingUnwindsBeforeStepping()
    {
        JogDial jog;
        QCOMPARE(roll(jog, 0, 30), 1); // 10 degrees left over
        QCOMPARE(roll(jog, 25, 15), 0);
        QCOMPARE(roll(jog, 10, -10), -1);
    }

    void wrapsAroundTheSeam()
    {
        JogDial jog;
        // atan2 jumps from +180 to -180 on the left side of the stick.
        QCOMPARE(roll(jog, 170, 255), 4);
    }

    void straightPushesDontStep()
    {
        JogDial jog;
        // Push right, release through the center, push left: never a rotation.
        for (qreal m = 0; m <= 1.0; m += 0.1)
            QCOMPARE(jog.update(rim(0, m)), 0);
        QCOMPARE(jog.update(QPointF()), 0);
        for (qreal m = 0; m <= 1.0; m += 0.1)
            QCOMPARE(jog.update(rim(180, m)), 0);
    }

    void snappingAcrossCenterIsIgnored()
    {
        JogDial jog;
        jog.update(rim(0));
        QCOMPARE(jog.update(rim(170)), 0);
    }

    void leavingTheRimResets()
    {
        JogDial jog;
        QCOMPARE(roll(jog, 0, 15), 0);
        jog.update(rim(15, 0.3)); // drifts toward center
        QCOMPARE(roll(jog, 15, 30), 0); // re-engages fresh, 15 degrees < 20
    }

    void configurableStep()
    {
        JogDial jog;
        jog.setStepDegrees(45);
        QCOMPARE(roll(jog, 0, 95), 2);
    }
};

QTEST_APPLESS_MAIN(TestJogDial)
#include "tst_jogdial.moc"
