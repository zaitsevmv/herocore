#include <greeting.hpp>

#include <userver/utest/utest.hpp>

using herocore::UserType;

UTEST(SayHelloTo, Basic) {
  EXPECT_EQ(herocore::SayHelloTo("Developer", UserType::kFirstTime),
            "Hello, Developer!\n");
  EXPECT_EQ(herocore::SayHelloTo({}, UserType::kFirstTime),
            "Hello, unknown user!\n");

  EXPECT_EQ(herocore::SayHelloTo("Developer", UserType::kKnown),
            "Hi again, Developer!\n");
  EXPECT_EQ(herocore::SayHelloTo({}, UserType::kKnown),
            "Hi again, unknown user!\n");
}