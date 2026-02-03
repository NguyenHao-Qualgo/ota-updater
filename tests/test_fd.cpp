#include <gtest/gtest.h>

#include "flash/fd.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace {

TEST(FdTests, ClosesFileDescriptorOnDestruct) {
    int fd = ::open("/dev/null", O_RDONLY);
    ASSERT_GE(fd, 0);

    {
        flash::Fd holder(fd);
        ASSERT_TRUE(holder.Valid());
        EXPECT_EQ(holder.Get(), fd);
    }

    errno = 0;
    int rc = ::close(fd);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(errno, EBADF);
}

} // namespace
