#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Maestre
 **
 ** LinkedIn: https://www.linkedin.com/in/francescmaestre/
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <AGitProcess.h>

/**
 * @brief Runs a git command and emits parsed commit records in batches as they arrive.
 *
 * Unlike GitRequestorProcess (which accumulates all output before emitting), this class
 * connects to readyReadStandardOutput and forwards data in batchSize-record chunks via
 * batchReady(). When the process exits, any remaining buffered data is flushed and
 * streamComplete() is emitted so the consumer can finalize its state.
 *
 * Intended for git log -z output where records are delimited by '\0'.
 */
class GitStreamingProcess : public AGitProcess
{
    Q_OBJECT

signals:
    void batchReady(QByteArray batch);
    void streamComplete();

public:
    static constexpr int kDefaultBatchSize = 2000;

    explicit GitStreamingProcess(GitRepoConfig config, int batchSize = kDefaultBatchSize);
    GitExecResult run(const QString& command) override;

private:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus) override;
    void accumulate(const QByteArray& data);

    QByteArray mBuffer;
    int mBatchSize;
    int mRecordCount = 0;
};
