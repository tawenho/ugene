/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2021 UniPro <ugene@unipro.ru>
 * http://ugene.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "GUITestLauncher.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QMap>
#include <QTextStream>
#include <QThread>

#include <U2Core/AppContext.h>
#include <U2Core/CMDLineCoreOptions.h>
#include <U2Core/CmdlineTaskRunner.h>
#include <U2Core/ExternalToolRegistry.h>
#include <U2Core/Timer.h>
#include <U2Core/U2OpStatusUtils.h>
#include <U2Core/U2SafePoints.h>

#include "GUITestTeamcityLogger.h"
#include "UGUITestBase.h"

#define GUITESTING_REPORT_PREFIX "GUITesting"

namespace U2 {

GUITestLauncher::GUITestLauncher(int suiteNumber, bool noIgnored, QString iniFileTemplate)
    : Task("gui_test_launcher", TaskFlags(TaskFlag_ReportingIsSupported) | TaskFlag_ReportingIsEnabled),
      suiteNumber(suiteNumber), noIgnored(noIgnored), pathToSuite(""), iniFileTemplate(iniFileTemplate) {
    tpm = Task::Progress_Manual;
    testOutDir = getTestOutDir();

    QWidget *splashScreen = QApplication::activeWindow();
    if (splashScreen != nullptr) {
        splashScreen->hide();
    }
}

GUITestLauncher::GUITestLauncher(QString _pathToSuite, bool _noIgnored, QString _iniFileTemplate)
    : Task("gui_test_launcher", TaskFlags(TaskFlag_ReportingIsSupported) | TaskFlag_ReportingIsEnabled),
      suiteNumber(0), noIgnored(_noIgnored), pathToSuite(_pathToSuite), iniFileTemplate(_iniFileTemplate) {
    tpm = Task::Progress_Manual;
    testOutDir = getTestOutDir();
}

bool GUITestLauncher::renameTestLog(const QString &testName) {
    QString outFileName = testOutFile(testName);
    QString outFilePath = testOutDir + QString("/logs/");

    QFile outLog(outFilePath + outFileName);
    return outLog.rename(outFilePath + "failed_" + outFileName);
}

void GUITestLauncher::run() {
    if (!initTestList()) {
        // FIXME: if test suite can't run for some reason UGENE runs shutdown task that asserts that startup is in progress.
        //  Workaround: wait 3 seconds to ensure that startup is complete & GUI test base error message is printed.
        QThread::currentThread()->sleep(3);
        return;
    }
    if (testList.isEmpty()) {
        setError(tr("No tests to run"));
        return;
    }

    qint64 suiteStartMicros = GTimer::currentTimeMicros();

    int finishedCount = 0;
    for (GUITest *test : qAsConst(testList)) {
        if (isCanceled()) {
            return;
        }
        if (test == nullptr) {
            updateProgress(finishedCount++);
            continue;
        }
        QString fullTestName = test->getFullName();
        QString teamcityTestName = UGUITest::getTeamcityTestName(test->suite, test->name);
        testResultByFullTestNameMap[fullTestName] = "";

        firstTestRunCheck(fullTestName);

        bool isValidPlatform = UGUITestLabels::hasPlatformLabel(test);
        bool isIgnored = UGUITestLabels::hasIgnoredLabel(test);
        bool isSkipIgnoredCheck = noIgnored;
        if (isValidPlatform && (!isIgnored || isSkipIgnoredCheck)) {
            qint64 startTime = GTimer::currentTimeMicros();
            GUITestTeamcityLogger::testStarted(teamcityTestName);

            try {
                QString testResult = runTest(fullTestName, test->timeout);
                testResultByFullTestNameMap[fullTestName] = testResult;
                if (GUITestTeamcityLogger::isTestFailed(testResult)) {
                    renameTestLog(fullTestName);
                }

                qint64 finishTime = GTimer::currentTimeMicros();
                GUITestTeamcityLogger::teamCityLogResult(teamcityTestName, testResult, GTimer::millisBetween(startTime, finishTime));
            } catch (const std::exception &exc) {
                coreLog.error("Got exception while running test: " + fullTestName);
                coreLog.error("Exception text: " + QString(exc.what()));
            }
        } else if (isValidPlatform) {
            // If the test should run on the current platform but has ignored label -> report it to the teamcity.
            GUITestTeamcityLogger::testIgnored(teamcityTestName, test->getDescription());
        }

        updateProgress(finishedCount++);
    }
    qint64 suiteEndMicros = GTimer::currentTimeMicros();
    qint64 suiteTimeMinutes = ((suiteEndMicros - suiteStartMicros) / 1000000) / 60;
    coreLog.info(QString("Suite %1 finished in %2 minutes").arg(suiteNumber).arg(suiteTimeMinutes));
}

void GUITestLauncher::firstTestRunCheck(const QString &testName) {
    QString testResult = testResultByFullTestNameMap[testName];
    Q_ASSERT(testResult.isEmpty());
}

/** Returns ideal tests list for the given suite or an empty list if there is no ideal configuration is found. */
QList<GUITest *> getIdealNightlyTestsSplit(int suiteIndex, int suiteCount, const QList<GUITest *> &allTests) {
    QList<int> testsPerSuite;
    if (suiteCount == 3) {
        testsPerSuite << 910 << 750 << -1;
    } else if (suiteCount == 4) {
        testsPerSuite << 640 << 680 << 640 << -1;
    } else if (suiteCount == 5) {
        testsPerSuite << 540 << 575 << 465 << 535 << -1;
    }
    QList<GUITest *> tests;
    if (testsPerSuite.size() == suiteCount) {
        SAFE_POINT(testsPerSuite.size() == suiteCount, QString("Illegal testsPerSuite size: %1").arg(testsPerSuite.size()), tests);
        int offset = 0;
        for (int i = 0; i < suiteIndex; i++) {
            offset += testsPerSuite[i];
        }
        int testCount = testsPerSuite[suiteIndex];    // last index is -1 => list.mid(x, -1) returns a tail.
        tests << allTests.mid(offset, testCount);
    }
    return tests;
}

bool GUITestLauncher::initTestList() {
    testList.clear();

    UGUITestBase *guiTestBase = AppContext::getGUITestBase();
    SAFE_POINT(guiTestBase != nullptr, "Test base is NULL", false);

    // Label set to build a run-time test set is passed via environment variable.
    QString labelEnvVar = qgetenv("UGENE_GUI_TEST_LABEL");
    QStringList labelList = labelEnvVar.isEmpty() ? QStringList() : labelEnvVar.split(",");

    if (suiteNumber != 0) {
        // If no label is provided 'Nightly' (UGUITestLabels::Nightly) label is used by default.
        if (labelList.isEmpty()) {
            labelList << UGUITestLabels::Nightly;
        }
        int testSuiteCount = 1;
        if (labelList.contains(UGUITestLabels::Nightly)) {
            // TODO: make configurable via ENV.
            testSuiteCount = isOsWindows() ? 3
                             : isOsMac()   ? 4
                             : isOsLinux() ? 5
                                           : 1;
        }

        if (suiteNumber < 1 || suiteNumber > testSuiteCount) {
            setError(QString("Invalid suite number: %1. There are %2 suites").arg(suiteNumber).arg(testSuiteCount));
            return false;
        }

        QList<GUITest *> labeledTestList = guiTestBase->getTests(UGUITestBase::Normal, labelList);
        if (labelList.contains(UGUITestLabels::Nightly)) {
            testList = getIdealNightlyTestsSplit(suiteNumber - 1, testSuiteCount, labeledTestList);
        }
        if (testList.isEmpty()) {
            // If there is no ideal test split for the given number -> distribute tests between suites evenly.
            for (int i = suiteNumber - 1; i < labeledTestList.length(); i += testSuiteCount) {
                testList << labeledTestList[i];
            }
        }
        coreLog.info(QString("Running suite %1-%2, Tests in the suite: %3, total tests: %4").arg(labelList.join(",")).arg(suiteNumber).arg(testList.size()).arg(labeledTestList.length()));
    } else if (!pathToSuite.isEmpty()) {
        // If a file with tests is specified we ignore labels and look-up in the complete tests set.
        QList<GUITest *> allTestList = guiTestBase->getTests(UGUITestBase::Normal);
        QString absPath = QDir().absoluteFilePath(pathToSuite);
        QFile suite(absPath);
        if (!suite.open(QFile::ReadOnly)) {
            setError("Can't open suite file: " + absPath);
            return false;
        }
        char buf[1024];
        while (suite.readLine(buf, sizeof(buf)) != -1) {
            QString testName = QString(buf).remove('\n').remove('\r').remove('\t').remove(' ');
            if (testName.startsWith("#") || testName.isEmpty()) {
                continue;    // comment line or empty line.
            }
            bool added = false;
            for (GUITest *test : qAsConst(allTestList)) {
                QString fullTestName = test->getFullName();
                QString teamcityTestName = UGUITest::getTeamcityTestName(test->suite, test->name);
                if (testName == fullTestName || testName == teamcityTestName) {
                    testList << test;
                    added = true;
                    break;
                }
                if (testName == test->suite) {
                    testList << test;
                    added = true;
                }
            }
            if (!added) {
                setError("Test not found: " + testName);
                return false;
            }
        }
    } else {
        // Run all tests with the given list of labels as a single suite.
        // If the list of labels is empty all tests are selected.
        testList = guiTestBase->getTests(UGUITestBase::Normal, labelList);
    }

    return true;
}

void GUITestLauncher::updateProgress(int finishedCount) {
    int testsSize = testList.size();
    if (testsSize) {
        stateInfo.progress = finishedCount * 100 / testsSize;
    }
}

QString GUITestLauncher::testOutFile(const QString &testName) {
    return QString("ugene_" + testName + ".out").replace(':', '_');
}

QString GUITestLauncher::getTestOutDir() {
    QString date = QDate::currentDate().toString("dd.MM.yyyy");
    QString guiTestOutputDirectory = qgetenv("GUI_TESTING_OUTPUT");
    QString initPath;
    if (guiTestOutputDirectory.isEmpty()) {
        initPath = QDir::homePath() + "/gui_testing_output/" + date;
    } else {
        initPath = guiTestOutputDirectory + "/gui_testing_output/" + date;
    }
    QDir d(initPath);
    int i = 1;
    while (d.exists()) {
        d = QDir(initPath + QString("_%1").arg(i));
        i++;
    }
    return d.absolutePath();
}

static bool restoreTestDirWithExternalScript(const QString &pathToShellScript, const QString &iniFilePath) {
    QDir testsDir(qgetenv("UGENE_TESTS_PATH"));
    if (!testsDir.exists()) {
        coreLog.error("UGENE_TESTS_PATH is not set!");
        return false;
    }
    QDir dataDir(qgetenv("UGENE_DATA_PATH"));
    if (!dataDir.exists()) {
        coreLog.error("UGENE_DATA_PATH is not set!");
        return false;
    }

    QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
    processEnv.insert("UGENE_TESTS_DIR_NAME", testsDir.dirName());
    processEnv.insert("UGENE_DATA_DIR_NAME", dataDir.dirName());
    processEnv.insert("UGENE_USER_INI", iniFilePath);
    qint64 startTimeMicros = GTimer::currentTimeMicros();
    QProcess process;
    process.setProcessEnvironment(processEnv);
    QString restoreProcessWorkDir = QFileInfo(testsDir.absolutePath() + "/../").absolutePath();
    process.setWorkingDirectory(restoreProcessWorkDir);    // Parent dir of the test dir.
//    coreLog.info("Running restore process, work dir: " + restoreProcessWorkDir +
//                 ", tests dir: " + testsDir.dirName() +
//                 ", data dir: " + dataDir.dirName() +
//                 ", script: " + pathToShellScript);
#ifdef Q_OS_WIN
    process.start("cmd /C " + pathToShellScript);
#else
    process.start("/bin/bash", QStringList() << pathToShellScript);
#endif
    qint64 processId = process.processId();
    bool isStarted = process.waitForStarted();
    if (!isStarted) {
        coreLog.error("An error occurred while running restore script: " + process.errorString());
        return false;
    } else {
    }
    bool isFinished = process.waitForFinished(5000);

    qint64 endTimeMicros = GTimer::currentTimeMicros();
    qint64 runTimeMillis = (endTimeMicros - startTimeMicros) / 1000;
    coreLog.info("Backup and restore run time (millis): " + QString::number(runTimeMillis));

    QProcess::ExitStatus exitStatus = process.exitStatus();
    if (!isFinished || exitStatus != QProcess::NormalExit) {
        CmdlineTaskRunner::killChildrenProcesses(processId);
        coreLog.error("Backup restore script was killed/exited with bad status: " + QString::number(exitStatus));
        return false;
    }
    return true;
}

QProcessEnvironment GUITestLauncher::prepareTestRunEnvironment(const QString &testName, int testRunIteration) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QDir().mkpath(testOutDir + "/logs");
    env.insert(ENV_UGENE_DEV, "1");
    env.insert(ENV_GUI_TEST, "1");
    env.insert(ENV_USE_NATIVE_DIALOGS, "0");
    env.insert(U2_PRINT_TO_FILE, testOutDir + "/logs/" + testOutFile(testName));

    QString iniFilePath = testOutDir + "/inis/" + QString(testName).replace(':', '_') + "_run_" + QString::number(testRunIteration) + "_UGENE.ini";
    if (!iniFileTemplate.isEmpty() && QFile::exists(iniFileTemplate)) {
        QFile::copy(iniFileTemplate, iniFilePath);
    }
    env.insert(U2_USER_INI, iniFilePath);

    QString externalScriptToRestore = qgetenv("UGENE_TEST_EXTERNAL_SCRIPT_TO_RESTORE");
    if (!externalScriptToRestore.isEmpty()) {
        restoreTestDirWithExternalScript(externalScriptToRestore, iniFilePath);
    }

    return env;
}

QString GUITestLauncher::runTest(const QString &testName, const int timeout) {
    int maxReruns = qMax(qgetenv("UGENE_TEST_NUMBER_RERUN_FAILED_TEST").toInt(), 0);
    QString testOutput;
    bool isVideoRecordingOn = qgetenv("UGENE_TEST_ENABLE_VIDEO_RECORDING") == "1";
    for (int iteration = 0; iteration < 1 + maxReruns; iteration++) {
        if (iteration >= 1) {
            coreLog.error(QString("Re-running the test. Current re-run: %1, max re-runs: %2").arg(iteration).arg(maxReruns));
        }
        U2OpStatusImpl os;
        testOutput = runTestOnce(os, testName, iteration, timeout, isVideoRecordingOn && iteration > 0);
        bool isFailed = os.hasError() || GUITestTeamcityLogger::isTestFailed(testOutput);
        if (!isFailed) {
            break;
        }
        coreLog.error(QString("Test failed with error: '%1'. Test output is '%2'.").arg(os.getError()).arg(testOutput));
    }
    return testOutput;
}

QString GUITestLauncher::runTestOnce(U2OpStatus &os, const QString &testName, int iteration, const int timeout, bool enableVideoRecording) {
    QProcessEnvironment environment = prepareTestRunEnvironment(testName, iteration);

    QString path = QCoreApplication::applicationFilePath();
    QStringList arguments = getTestProcessArguments(testName);

    // ~QProcess is killing the process, will not return until the process is terminated.
    QProcess process;
    process.setProcessEnvironment(environment);
    process.start(path, arguments);
    qint64 processId = process.processId();

    QProcess screenRecorderProcess;
    if (enableVideoRecording) {
        screenRecorderProcess.start(getScreenRecorderString(testName));
    }

    bool isStarted = process.waitForStarted();
    if (!isStarted) {
        QString error = QString("An error occurred while starting UGENE: %1").arg(process.errorString());
        os.setError(error);
        return error;
    }
    bool isFinished = process.waitForFinished(timeout);
    QProcess::ExitStatus exitStatus = process.exitStatus();

    if (!isFinished || exitStatus != QProcess::NormalExit) {
        CmdlineTaskRunner::killChildrenProcesses(processId);
    }

#ifdef Q_OS_WIN
    QProcess::execute("closeErrorReport.exe");    //this exe file, compiled Autoit script
#endif

    QString testResult = readTestResult(process.readAllStandardOutput());

    if (enableVideoRecording) {
        screenRecorderProcess.close();
        bool isScreenRecorderFinished = screenRecorderProcess.waitForFinished(2000);
        if (!isScreenRecorderFinished) {
            screenRecorderProcess.kill();
            screenRecorderProcess.waitForFinished(2000);
        }
        if (!GUITestTeamcityLogger::isTestFailed(testResult)) {
            QFile(getVideoPath(testName)).remove();
        }
    }

    if (isFinished && exitStatus == QProcess::NormalExit) {
        return testResult;
    }
#ifdef Q_OS_WIN
    CmdlineTaskRunner::killProcessTree(process.processId());
    process.kill();    // to avoid QProcess: Destroyed while process is still running.
    process.waitForFinished(2000);
#endif
    QString error = isFinished ? QString("An error occurred while finishing UGENE: %1\n%2").arg(process.errorString()).arg(testResult) : QString("Test fails because of timeout.");
    os.setError(error);
    return error;
}

QStringList GUITestLauncher::getTestProcessArguments(const QString &testName) {
    return QStringList() << QString("--") + CMDLineCoreOptions::LAUNCH_GUI_TEST + "=" + testName;
}

QString GUITestLauncher::readTestResult(const QByteArray &output) {
    QString msg;
    QTextStream stream(output, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        QString str = stream.readLine();

        if (str.contains(GUITESTING_REPORT_PREFIX)) {
            msg = str.remove(0, str.indexOf(':') + 1);
            if (!msg.isEmpty()) {
                break;
            }
        }
    }

    return msg;
}

QString GUITestLauncher::generateReport() const {
    QString res;
    res += "<table width=\"100%\">";
    res += QString("<tr><th>%1</th><th>%2</th></tr>").arg(tr("Test name")).arg(tr("Status"));

    QMap<QString, QString>::const_iterator i;
    for (i = testResultByFullTestNameMap.begin(); i != testResultByFullTestNameMap.end(); ++i) {
        QString color = "green";
        if (GUITestTeamcityLogger::isTestFailed(i.value())) {
            color = "red";
        }
        res += QString("<tr><th><font color='%3'>%1</font></th><th><font color='%3'>%2</font></th></tr>").arg(i.key()).arg(i.value()).arg(color);
    }
    res += "</table>";

    return res;
}

QString GUITestLauncher::getScreenRecorderString(QString testName) {
    QString result;
#ifdef Q_OS_LINUX
    QRect rec = QApplication::desktop()->screenGeometry();
    int height = rec.height();
    int width = rec.width();
    QString display = qgetenv("DISPLAY");
    result = QString("ffmpeg -video_size %1x%2 -framerate 5 -f x11grab -i %3.0 %4").arg(width).arg(height).arg(display).arg(getVideoPath(testName));
#elif defined Q_OS_MAC
    result = QString("ffmpeg -f avfoundation -r 5 -i \"1:none\" \"%1\"").arg(getVideoPath(testName));
#elif defined Q_OS_WIN
    result = QString("ffmpeg -f dshow -i video=\"UScreenCapture\" -r 5 %1").arg(getVideoPath(testName.replace(':', '_')));
#endif
    uiLog.trace("going to record video: " + result);
    return result;
}

QString GUITestLauncher::getVideoPath(const QString &testName) {
    QDir().mkpath(QDir::currentPath() + "/videos");
    QString result = QDir::currentPath() + "/videos/" + testName + ".avi";
    return result;
}
}    // namespace U2
