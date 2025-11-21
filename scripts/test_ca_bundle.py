#!/usr/bin/env python3
"""
Test script to verify the CA certificate bundle
This script can be used to validate the generated certificate bundle
"""

import struct
import sys
from pathlib import Path

def analyze_bundle(bundle_path):
    """Analyze the certificate bundle and display statistics"""
    
    if not Path(bundle_path).exists():
        print(f"Error: Bundle file {bundle_path} not found")
        return False
    
    try:
        with open(bundle_path, 'rb') as f:
            data = f.read()
        
        if len(data) < 2:
            print("Error: Bundle file too small")
            return False
        
        # Read number of certificates (first 2 bytes, big-endian)
        num_certs = struct.unpack('>H', data[:2])[0]
        print(f"Certificate Bundle Analysis:")
        print(f"  File: {bundle_path}")
        print(f"  Size: {len(data):,} bytes")
        print(f"  Number of certificates: {num_certs}")
        
        # Calculate average certificate size
        if num_certs > 0:
            avg_size = (len(data) - 2) / num_certs
            print(f"  Average certificate size: {avg_size:.1f} bytes")
        
        # Verify bundle structure
        offset = 2
        valid_certs = 0
        
        for i in range(num_certs):
            if offset + 4 > len(data):
                print(f"Error: Unexpected end of data at certificate {i}")
                break
            
            # Read name length and key length
            name_len, key_len = struct.unpack('>HH', data[offset:offset+4])
            offset += 4
            
            if offset + name_len + key_len > len(data):
                print(f"Error: Certificate {i} data extends beyond file")
                break
            
            # Skip the actual certificate data
            offset += name_len + key_len
            valid_certs += 1
        
        print(f"  Valid certificates parsed: {valid_certs}")
        
        if valid_certs == num_certs:
            print("  ✓ Bundle structure is valid")
            return True
        else:
            print("  ✗ Bundle structure has errors")
            return False
            
    except Exception as e:
        print(f"Error analyzing bundle: {e}")
        return False

def compare_bundles(old_bundle, new_bundle):
    """Compare two certificate bundles"""
    
    print("\nBundle Comparison:")
    
    for name, path in [("Old", old_bundle), ("New", new_bundle)]:
        if Path(path).exists():
            size = Path(path).stat().st_size
            print(f"  {name} bundle: {size:,} bytes")
        else:
            print(f"  {name} bundle: Not found")
    
    # Calculate size difference
    if Path(old_bundle).exists() and Path(new_bundle).exists():
        old_size = Path(old_bundle).stat().st_size
        new_size = Path(new_bundle).stat().st_size
        diff = new_size - old_size
        percent = (diff / old_size) * 100 if old_size > 0 else 0
        
        print(f"  Size difference: {diff:+,} bytes ({percent:+.1f}%)")

def main():
    """Main test function"""
    
    # Default paths
    bundle_path = "../data/cert/x509_crt_bundle.bin"
    backup_path = "../data/cert/x509_crt_bundle.bin.backup"
    
    if len(sys.argv) > 1:
        bundle_path = sys.argv[1]
    
    print("CA Certificate Bundle Test")
    print("=" * 40)
    
    # Analyze the current bundle
    success = analyze_bundle(bundle_path)
    
    # Compare with backup if it exists
    if Path(backup_path).exists():
        compare_bundles(backup_path, bundle_path)
    
    # Test recommendations
    print("\nTesting Recommendations:")
    print("1. Build the firmware with the new certificate bundle")
    print("2. Test TLS connections to these common services:")
    print("   - https://www.google.com (Google Trust Services)")
    print("   - https://www.amazon.com (Amazon Trust Services)")
    print("   - https://www.microsoft.com (DigiCert)")
    print("   - https://httpbin.org (Let's Encrypt/ISRG)")
    print("3. Monitor device logs for certificate validation errors")
    print("4. Test your application's specific backend services")
    
    if success:
        print("\n✓ Certificate bundle appears to be valid")
        return 0
    else:
        print("\n✗ Certificate bundle validation failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
