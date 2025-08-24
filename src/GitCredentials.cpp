#include "GitCredentials.h"

#include <GitBase.h>
#include <GitConfig.h>

#include <QProcess>
#include <QTextStream>

void GitCredentials::configureStorage(const QString &user, const QString &password,
                                      const QSharedPointer<GitBase> &gitBase)
{
   const auto dir = gitBase->getGitDir();
   const auto ret = gitBase->run(QString("git config credential.helper \'store --file %1/.git-credentials\'").arg(dir));

   QProcess storeProcess;
   storeProcess.start("git", { "credential-store", "store", "--file", QString("%1/.git-credentials").arg(dir) });

   auto started = storeProcess.waitForStarted();

   if (started)
   {
      QScopedPointer<GitConfig> gitConf(new GitConfig(gitBase));

      QTextStream out(&storeProcess);
      out << "protocol=https\n";
      out << "host=" << gitConf->getServerHost().toUtf8() << '\n';
      out << "username=" << user.toUtf8() << '\n';
      out << "password=" << password.toUtf8() << '\n';
      out << '\n';

      storeProcess.closeWriteChannel();
      storeProcess.waitForFinished();
   }
}

void GitCredentials::configureCache(uint64_t timeout, const QSharedPointer<GitBase> &gitBase)
{
   gitBase->run("git credential-cache exit");
   gitBase->run(
       QString("git credential.helper 'cache --timeout %1 --socket %2/socket'").arg(timeout).arg(gitBase->getGitDir()));
}
