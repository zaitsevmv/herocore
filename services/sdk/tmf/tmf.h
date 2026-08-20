#pragma once

#include <filesystem>

#include <include/async/async_task.h>
#include <sdk/proto/tmf.pb.h>

namespace NSdk {

NAsync::TAsyncTask<bool> CreateServer(const TCreateServerRequest request);

NAsync::TAsyncTask<bool> StartServer(const TStartServerRequest request);

NAsync::TAsyncTask<bool> RemoveServer(const TRemoveServerRequest request);

NAsync::TAsyncTask<bool> StopServer(const TStopServerRequest request);

NAsync::TAsyncTask<std::string> GetServerConfig(const TGetServerConfigRequest request);

NAsync::TAsyncTask<bool> WriteServerConfig(const TWriteServerConfigRequest request);

NAsync::TAsyncTask<std::filesystem::path> GetMapsMount(const TGetMapsMountRequest request);

NAsync::TAsyncTask<std::string> GetServerConfig(const TGetServerConfigRequest request);

NAsync::TAsyncTask<bool> WriteServerConfig(const TWriteServerConfigRequest request);

NAsync::TAsyncTask<bool> UploadServerFile(const TUploadServerFileRequest request);

NAsync::TAsyncTask<bool> DeleteServerFile(const TDeleteServerFileRequest request);

} // NSdk
