// startup_banner.hh

#pragma once

namespace coralmicro {

    const char* PROJECT_NAME = "CoralMicro In-Tree Andon System";
    constexpr const char* PROJECT_LOGO = R"(٩(͡๏̯͡๏)۶)";
    const char* DEVELOPER_NAME = "JC";

    void print_startup_banner() {
        printf("\r\n");
        printf("========================================\r\n");
        printf("= %s =\r\n", PROJECT_NAME);
        printf("= %s =\r\n", PROJECT_LOGO);
        printf("= Developer: %s =\r\n", DEVELOPER_NAME);
        printf("========================================\r\n");
        printf("\r\n");
    }

}