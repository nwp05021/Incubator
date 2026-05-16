# AWS IoT Core certificates

Replace the placeholder files in this directory before enabling `INCUBATOR_ENABLE_CLOUD`.

- `aws-root-ca.pem`: Amazon Root CA 1 PEM
- `device-certificate.pem.crt`: device certificate PEM from AWS IoT Core
- `private.pem.key`: device private key PEM from AWS IoT Core

The current PlatformIO `espidf, arduino` mixed build keeps the active placeholders in
`src/cloud/AwsIotCredentials.h` for reliable linking. These files are retained as
the expected PEM source material if you later switch to IDF-only `EMBED_TXTFILES`
or load credentials from NVS/LittleFS.
