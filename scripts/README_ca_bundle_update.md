# CA Certificate Bundle Update Process

This document describes the process for updating the TLS certificate chain (CA bundle) used by the ESP32 controller firmware.

## Overview

The ESP32 controller uses a custom certificate bundle format that stores only the subject name and public key of trusted Certificate Authorities (CAs) to reduce space requirements. This bundle is embedded into the firmware and used for TLS/SSL connections.

## Files Involved

- `scripts/gen_crt_bundle.py` - Python script to generate the certificate bundle
- `scripts/cmn_crt_authorities.csv` - Filter file containing the *minimal* CA set we keep (no separate “full” list is retained)
- `scripts/cacrt_all.pem` - Mozilla's CA certificate bundle (downloaded; the only raw PEM source on disk)
- `data/cert/x509_crt_bundle.bin` - Generated binary certificate bundle (embedded in firmware; supersedes any temporary `x509_crt_bundle` file)

## Update Process

### 1. Download Latest CA Certificates

Download the latest Mozilla CA certificate bundle:

```bash
cd scripts/
curl -o cacrt_all.pem https://curl.se/ca/cacert.pem
```

### 2. Review and Update CA Filter List

The `cmn_crt_authorities.csv` file contains a curated list of common Certificate Authorities based on clear inclusion criteria.

#### Inclusion Criteria

**1. Major Cloud Providers (Always Included - No Market Share Requirement)**
- **Amazon Trust Services** - All root certificates
- **Google Trust Services (GTS)** - All root certificates  
- **Microsoft Corporation** - All root certificates (Azure services)

These are included regardless of market share because they are essential for cloud service connectivity.

**2. High Market Share CAs (Based on w3techs.com SSL Certificate Statistics)**
CAs with significant market presence (>1% market share or top 15 globally) included as backup:

- **Let's Encrypt (ISRG)** - ~53.4% market share (2024) - ISRG Root X1, ISRG Root X2
- **IdenTrust** - ~16.8% market share - IdenTrust Commercial Root CA 1, IdenTrust Public Sector Root CA 1
- **GlobalSign** - ~12.0% market share - GlobalSign Root CA - R3, GlobalSign Root CA - R6, GlobalSign ECC Root CA - R5
- **Sectigo (formerly COMODO)** - ~7.6% market share - COMODO RSA/ECC, USERTrust RSA/ECC
- **DigiCert Group** - ~5.5% market share - DigiCert Trusted Root G4, DigiCert TLS ECC P384 Root G5, DigiCert TLS RSA4096 Root G5, DigiCert Global Root G3
- **GoDaddy Group** - ~4.5% market share - Go Daddy Root Certificate Authority - G2, Starfield Root Certificate Authority - G2
- **Entrust** - Significant enterprise presence - Entrust Root Certification Authority, Entrust Root Certification Authority - G2
- **Certum** - European market leader - Certum Trusted Network CA, Certum Trusted Network CA 2
- **QuoVadis** - Established CA with global trust - QuoVadis Root CA 2, QuoVadis Root CA 2 G3, QuoVadis Root CA 3 G3
- **Trustwave** - Enterprise security focus - Trustwave Global Certification Authority, Trustwave Global ECC P256/P384

**3. Certificate Availability**
- Only certificates that exist in the current `cacrt_all.pem` file are included
- Deprecated, expired, or removed certificates are automatically excluded
- When updating, verify all certificates in the CSV exist in the latest Mozilla bundle

#### Exclusion Criteria

**Automatically Excluded:**
- Certificates not present in `cacrt_all.pem` (deprecated/removed by Mozilla)
- Regional CAs with <1% global market share
- Country-specific CAs with limited international scope
- Legacy/deprecated certificate versions replaced by newer roots

**Manual Review Required:**
- CAs with security incidents or trust issues
- CAs that no longer meet modern cryptographic standards
- Niche industry-specific CAs not relevant to IoT/embedded use cases

### 3. Generate New Certificate Bundle

Run the certificate bundle generation script:

```bash
cd scripts/
python3 gen_crt_bundle.py --input cacrt_all.pem --filter cmn_crt_authorities.csv
```

This will:
- Parse the Mozilla CA bundle (`cacrt_all.pem`)
- Filter certificates based on the CSV file
- Generate a compressed binary bundle (`x509_crt_bundle`)

### 4. Install the New Bundle

Move the generated bundle to the correct location:

```bash
mv x509_crt_bundle ../data/cert/x509_crt_bundle.bin
rm -f x509_crt_bundle
```

> Note: The `x509_crt_bundle` file only exists as a temporary build artifact inside `scripts/`. After the `mv` (or explicit `rm`), no copy remains in the repository—only `data/cert/x509_crt_bundle.bin` is checked in and embedded.

### 5. Verify Bundle Integration

The certificate bundle is automatically embedded into the firmware during build via PlatformIO configuration:

```ini
board_build.embed_files = data/cert/x509_crt_bundle.bin
```

The bundle is accessed in the code via:

```cpp
extern const uint8_t rootca_crt_bundle_start[] asm(
    "_binary_data_cert_x509_crt_bundle_bin_start");
extern const uint8_t rootca_crt_bundle_end[] asm(
    "_binary_data_cert_x509_crt_bundle_bin_end");
```

## Testing

### Staging Environment Testing

Before deploying to production:

1. Build firmware with the new certificate bundle
2. Deploy to staging devices
3. Test TLS connections to various services:
   - Cloud APIs (AWS, Google Cloud, Azure)
   - Web services using common CAs
   - Your application's backend servers
4. Monitor for certificate validation errors
5. Verify no trusted connections are broken

### Production Deployment

1. Ensure staging tests pass completely
2. Deploy firmware update to production devices
3. Monitor device connectivity and error logs
4. Have rollback plan ready if issues occur

## Bundle Statistics

Current bundle (as of update):
- **Total certificates**: 37 CAs
- **Bundle size**: ~16.5KB (compressed binary format)
- **Coverage**: Major cloud providers plus top-market-share public CAs retained for fallback connectivity

## Maintenance Schedule

- **Regular updates**: Every 6 months or when major CA changes occur
- **Emergency updates**: When critical CA compromises or revocations happen
- **Review cycle**: Annual review of included CAs for relevance and security

## Troubleshooting

### Common Issues

1. **Certificate validation failures**: Check if the required CA is in the filter list
2. **Bundle too large**: Remove less critical CAs from the CSV filter
3. **Build errors**: Ensure the bundle file is in the correct location and format

### Validating Certificate List

Before generating the bundle, validate that all certificates in the CSV exist in the PEM file:

```bash
cd scripts/
python3 << 'EOF'
import csv
import re

def get_cert_names(filename):
    with open(filename, 'r') as f:
        content = f.read()
    names = re.findall(r'^([A-Z][^\n]+)\n=+\n-----BEGIN CERTIFICATE', content, re.MULTILINE)
    return set(names)

available = get_cert_names('cacrt_all.pem')

with open('cmn_crt_authorities.csv', 'r') as f:
    reader = csv.reader(f)
    next(reader)  # Skip header
    missing = []
    for row in reader:
        if len(row) >= 2 and row[1] not in available:
            missing.append(row[1])

if missing:
    print(f"ERROR: {len(missing)} certificates not found in cacrt_all.pem:")
    for cert in missing:
        print(f"  - {cert}")
    exit(1)
else:
    print("✓ All certificates in CSV are present in cacrt_all.pem")
EOF
```

### Adding New CAs

To add a new CA to the bundle:

1. Verify the CA meets inclusion criteria (Major Cloud Provider, >1% market share, or top 15 globally)
2. Verify the CA is in the Mozilla bundle (`cacrt_all.pem`)
3. Add the CA name to `cmn_crt_authorities.csv` with format: `Owner,Common Name or Certificate Name`
4. Validate using the script above
5. Regenerate the bundle following the process above

### Removing CAs

CAs are automatically removed when:
- They are no longer present in `cacrt_all.pem` (deprecated/removed by Mozilla)
- They fall below the market share threshold (<1% and not in top 15)

To manually remove a CA:

1. Remove the CA line from `cmn_crt_authorities.csv`
2. Validate the CSV
3. Regenerate the bundle
4. Test thoroughly to ensure no services are affected

## Security Considerations

- Only include CAs from reputable sources (Mozilla's trusted root store)
- Regularly review CA security incidents and remove compromised CAs
- Keep the filter list minimal to reduce attack surface
- Monitor CA certificate transparency logs for suspicious activity

## References

- [Mozilla CA Certificate Store](https://wiki.mozilla.org/CA)
- [ESP-IDF Certificate Bundle Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_crt_bundle.html)
- [curl CA Bundle](https://curl.se/docs/caextract.html)
