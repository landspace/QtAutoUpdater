#include "testinstaller.h"
#include <plugintest.h>

TestInstaller::TestInstaller(QObject *parent) :
	QObject{parent}
{}

TestInstaller::~TestInstaller()
{
	cleanup();
}

bool TestInstaller::isInstalled() const
{
	return _installed;
}

bool TestInstaller::setup()
{
	TEST_WRAP_BEGIN

	QVERIFY(QDir{_dir.path()}.mkpath(QStringLiteral("Formula")));
	QVERIFY(runGit({
					   QStringLiteral("init")
				   }));
	QVERIFY(runBrew({
						QStringLiteral("tap"),
						QStringLiteral("Skycoder42/QtAutoUpdaterTest"),
						QUrl::fromLocalFile(_dir.filePath(QStringLiteral(".git"))).toString(QUrl::FullyEncoded)
					}));

	QVERIFY(uninstall());

	TEST_WRAP_END
}

bool TestInstaller::cleanup()
{
	TEST_WRAP_BEGIN

	if (_installed)
		QVERIFY(uninstall());
	QVERIFY(runBrew({
						QStringLiteral("untap"),
						QStringLiteral("Skycoder42/QtAutoUpdaterTest")
					}, false));

	TEST_WRAP_END
}

void TestInstaller::setVersion(QVersionNumber version)
{
	_version = std::move(version);
}

bool TestInstaller::package()
{
	TEST_WRAP_BEGIN

	QFile formula{_dir.filePath(QStringLiteral("Formula/qtautoupdatertestpackage.rb"))};
	QVERIFY(formula.open(QIODevice::WriteOnly | QIODevice::Text));
	formula.write(FormulaTemplate.arg(_version.toString()).toUtf8());
	formula.close();

	QVERIFY(runGit({
					   QStringLiteral("add"),
					   QDir{_dir.path()}.relativeFilePath(formula.fileName())
				   }));
	QVERIFY(runGit({
					   QStringLiteral("commit"),
					   QStringLiteral("-m"),
					   QStringLiteral("Updated test formula to %1").arg(_version.toString())
				   }));

	TEST_WRAP_END
}

bool TestInstaller::install()
{
	TEST_WRAP_BEGIN

	QVERIFY2(!_installed, "You must run uninstall before installing again");
	QVERIFY(runBrew({
						QStringLiteral("update")
					}));
	QVERIFY(runBrew({
						QStringLiteral("install"),
						QStringLiteral("qtautoupdatertestpackage")
					}));
	_installed = true;

	TEST_WRAP_END
}

bool TestInstaller::uninstall()
{
	TEST_WRAP_BEGIN

	QVERIFY(runBrew({
						QStringLiteral("uninstall"),
						QStringLiteral("--force"),
						QStringLiteral("qtautoupdatertestpackage")
					}, _installed));
	_installed = false;

	TEST_WRAP_END
}

bool TestInstaller::runGit(const QStringList &arguments)
{
	TEST_WRAP_BEGIN

	const auto gitExe = QStandardPaths::findExecutable(QStringLiteral("git"));
	QVERIFY(!gitExe.isEmpty());
	QVERIFY(runTool(gitExe, arguments, _dir.path(), true));

	TEST_WRAP_END
}

bool TestInstaller::runBrew(const QStringList &arguments, bool verify)
{
	TEST_WRAP_BEGIN

	const auto brewExe = QStandardPaths::findExecutable(QStringLiteral("brew"));
	QVERIFY(!brewExe.isEmpty());
	QVERIFY(runTool(brewExe, arguments, {}, verify));

	TEST_WRAP_END
}

bool TestInstaller::runTool(const QString &program, const QStringList &arguments, const QString &workingDir, bool verify)
{
	TEST_WRAP_BEGIN

	QProcess proc;
	proc.setProgram(program);
	proc.setArguments(arguments);
	if (!workingDir.isEmpty())
		proc.setWorkingDirectory(workingDir);
	proc.setProcessChannelMode(QProcess::ForwardedChannels);
	proc.setStandardInputFile(QProcess::nullDevice());

	qDebug() << "Executing" << program << arguments;
	proc.start(QIODevice::ReadOnly);
	const auto procOk = proc.waitForFinished();
	QVERIFY2(procOk, qUtf8Printable(proc.errorString()));
	if (verify)
		QCOMPARE(proc.exitCode(), EXIT_SUCCESS);
	else if (proc.exitCode() != EXIT_SUCCESS)
		qWarning() << qUtf8Printable(program) << "process exited with unclean exit code:" << proc.exitCode();

	TEST_WRAP_END
}
