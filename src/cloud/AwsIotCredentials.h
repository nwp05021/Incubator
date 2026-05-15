#pragma once

namespace incubator::cloud
{
    inline constexpr const char* kAwsRootCaPem = R"PEM(
-----BEGIN CERTIFICATE-----
Replace this block with AmazonRootCA1.pem from AWS IoT Core.
-----END CERTIFICATE-----
)PEM";

    inline constexpr const char* kAwsDeviceCertPem = R"PEM(
-----BEGIN CERTIFICATE-----
Replace this block with the device certificate downloaded from AWS IoT Core.
-----END CERTIFICATE-----
)PEM";

    inline constexpr const char* kAwsPrivateKeyPem = R"PEM(
-----BEGIN PRIVATE KEY-----
Replace this block with the device private key downloaded from AWS IoT Core.
-----END PRIVATE KEY-----
)PEM";
}
