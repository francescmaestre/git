#include "GitStreamingProcess.h"

GitStreamingProcess::GitStreamingProcess(GitRepoConfig config, int batchSize)
    : AGitProcess(std::move(config))
    , mBatchSize(batchSize)
{
    connect(this, &AGitProcess::procDataReady, this, &GitStreamingProcess::accumulate, Qt::DirectConnection);
}

GitExecResult GitStreamingProcess::run(const QString& command)
{
    return { execute(command), {} };
}

void GitStreamingProcess::accumulate(const QByteArray& data)
{
    mBuffer.append(data);

    int scanPos = 0;
    while (scanPos < mBuffer.size()) {
        const int end = mBuffer.indexOf('\0', scanPos);
        if (end == -1)
            break;

        scanPos = end + 1;

        if (++mRecordCount >= mBatchSize) {
            emit batchReady(mBuffer.left(scanPos));
            mBuffer = mBuffer.mid(scanPos);
            scanPos = 0;
            mRecordCount = 0;
        }
    }
}

void GitStreamingProcess::onFinished(int, QProcess::ExitStatus)
{
    if (!mCanceling) {
        if (!mBuffer.isEmpty())
            emit batchReady(std::move(mBuffer));
        emit streamComplete();
    }
    deleteLater();
}
