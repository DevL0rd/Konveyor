#include "audio/audiocapture.h"

#include <QTest>

using namespace Konveyor::Audio;

class TestAudioCaptureNoPlugins : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { qputenv("SPA_PLUGIN_DIR", "/nonexistent/konveyor-spa"); }

    void theCaptureThreadEndsWhenPipeWireCannotMakeALoop()
    {
        AudioCapture capture(1.4F);
        QTRY_VERIFY_WITH_TIMEOUT(capture.hasFinished(), 10000);
        capture.setWanted({});
        capture.setBeatThreshold(2.0F);
    }
};

QTEST_GUILESS_MAIN(TestAudioCaptureNoPlugins)
#include "test_audiocapture_noplugins.moc"
