#pragma once

#include <include/async/async_task.h>

#include "tmf.pb.h"

namespace NSdk {

NAsync::TAsyncTask<bool> CreateServer(const TCreateServerRequest request);

} // NSdk
