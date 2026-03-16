# Software Bill of Materials (SBOM) Guide

## Overview

This document describes how to generate and use Software Bill of Materials (SBOM) for the Agentino project to support EU Cyber Resilience Act (CRA) compliance.

## What is an SBOM?

A Software Bill of Materials (SBOM) is a comprehensive inventory of all components, libraries, and dependencies used in a software product. It helps with:

- **Transparency**: Understanding what's in your software
- **Security**: Identifying vulnerable components
- **Compliance**: Meeting regulatory requirements like the CRA
- **Risk Management**: Assessing supply chain risks

## Pre-Generated SBOM

Agentino provides pre-generated SBOMs in both major formats:

### CycloneDX Format
A basic SBOM for Agentino is provided in [`sbom.json`](sbom.json) in CycloneDX 1.5 format. This includes:

- Agentino framework information
- Main runtime dependencies (Qt Framework, ImtCore, ACF, OpenSSL, Quazip)
- License information
- Project metadata

### SPDX Format
An SPDX 2.3 format SBOM is provided in [`sbom.spdx.json`](sbom.spdx.json). This includes:

- Agentino framework information
- Main runtime dependencies
- License information (using SPDX license identifiers)
- Package relationships

**Important Notes**:
- The version in both SBOM files uses `1.0.0-dev` as a placeholder
- **Maintainers should update both files with each release** to reflect:
  - Current Agentino version number
  - Creation timestamps (especially in SPDX format)
  - Document namespace UUID for SPDX (generate a new UUID for each release)
  - Specific Qt version used in testing
  - Any dependency changes
- Both SBOMs should be regenerated or updated before each release

**Update Schedule**:
- Per Release: Update version numbers, timestamps, UUIDs, and dependency information
- As Needed: When dependencies are added, removed, or updated
- Quarterly: Verify all information is current

## SBOM Standards

Agentino supports the following SBOM formats:

1. **CycloneDX** (Recommended)
   - Format: JSON or XML
   - Version: 1.5
   - Specification: https://cyclonedx.org/

2. **SPDX** (Alternative)
   - Format: JSON, YAML, or RDF
   - Version: 2.3
   - Specification: https://spdx.dev/

## Generating SBOM for Your Deployment

If you're deploying Agentino in your environment, you need to generate an SBOM that includes Agentino and all its dependencies.

### Method 1: Using CycloneDX Tools

#### For CMake Projects

1. Install CycloneDX CMake plugin:
```bash
# Add to your CMakeLists.txt
find_package(CycloneDX QUIET)
if(CycloneDX_FOUND)
    cyclonedx_generate_sbom(TARGET your_target)
endif()
```

2. Generate SBOM:
```bash
cmake --build . --target cyclonedx-sbom
```

#### For C++ Projects with Conan

```bash
# Install the plugin
pip install cyclonedx-conan

# Generate SBOM
conan install . --install-folder=build
cyclonedx-conan sbom -o sbom.json
```

### Method 2: Using SPDX Tools

#### Using spdx-sbom-generator

```bash
# Install the tool
go install github.com/opensbom-generator/spdx-sbom-generator@latest

# Generate SBOM
spdx-sbom-generator
```

### Method 3: Manual SBOM Creation

For custom setups, you can manually create an SBOM:

1. Start with the Agentino SBOM template: [`sbom.json`](sbom.json)
2. Add your deployment metadata
3. Include all dependencies with:
   - Name and version
   - License information
   - Download location or package URL
   - Checksums (SHA-256 recommended)
4. Document the dependency tree

## SBOM Components for Agentino

### Core Dependencies

**Qt Framework (Required)**
- Component: Qt
- Version: 6.x (typically 6.8 or later)
- License: Commercial (required for commercial use of Agentino)
- PURL: pkg:generic/qt@6.8 (example)
- Website: https://www.qt.io/
- Purpose: Cross-platform application framework
- Required Components: Core, Widgets, QML, Quick, QuickWidgets, Network, WebSockets, SQL, Xml, Svg, Concurrent, QuickControls2, PrintSupport

**ImtCore Framework (Required)**
- Component: ImtCore
- Version: As specified by IMTCOREDIR
- License: Proprietary (ImagingTools GmbH)
- Purpose: Enterprise application framework providing service management, GraphQL, authentication, database abstraction, and UI components

**ACF - Application Component Framework (Required)**
- Component: ACF
- Version: Compatible version
- License: Commercial
- PURL: pkg:github/ImagingTools/Acf
- Website: https://github.com/ImagingTools/Acf
- Purpose: Component-based architecture and licensing

**OpenSSL (Required)**
- Component: OpenSSL
- Version: 1.1.x or 3.x
- License: Apache-2.0 (3.x) or OpenSSL License (1.1.x)
- Purpose: Security and encryption

**Quazip (Required)**
- Component: Quazip
- Version: As linked
- License: LGPL-2.1 (permissible for use in commercial applications)
- Purpose: ZIP compression/decompression

**Standard C++ Library (Required)**
- Component: Implementation-dependent (libstdc++, libc++, MSVC STL)
- License: Implementation-dependent
- Purpose: C++ standard library functionality

### Build-Time Dependencies

**CMake (Optional - for CMake builds)**
- Version: 3.26 or later
- License: BSD-3-Clause
- PURL: pkg:generic/cmake
- Purpose: Build system generator

**QMake (Optional - for QMake builds)**
- Part of Qt distribution
- License: Same as Qt
- Purpose: Qt-based build system

**Doxygen (Optional - for documentation)**
- License: GPL-2.0
- Purpose: API documentation generation

### Development Dependencies

When developing or building from source, additional tools may be needed:
- C++ compiler (GCC, Clang, MSVC with C++17 support)
- Git (for source control)
- Visual Studio (Windows)
- Inno Setup (for Windows installers)

## Agentino Components

The Agentino system consists of multiple libraries and executables:

**Core Libraries:**
- `agentinodata` - Core data models for services, agents, and status information
- `agentinogql` - Server-side GraphQL resolvers and subscriptions
- `agentgql` - Client-side GraphQL integration and queries
- `agentinoqml` - QML bindings for UI components

**Packages:**
- `AgentinoGqlPck` - GraphQL schema and type definitions
- `AgentGqlPck` - Client-side GraphQL package
- `AgentinoDataPck` - Component factories and data utilities
- `AgentinoLoc` - Localization and licensing functionality

**Executables:**
- `AgentinoServer` - Central server application with GUI
- `AgentinoClient` - Client application for service management
- `AgentinoAgent` - Agent service that runs on managed nodes
- `AgentinoClientServer` - Shared client-server communication

## Updating the SBOM

### When to Update

Update your SBOM when:
- Adding or removing dependencies
- Upgrading dependency versions
- Changing build configurations
- Discovering security vulnerabilities in dependencies
- Releasing a new version of Agentino

### Update Process

1. Review all dependencies
2. Update version numbers
3. Verify license information
4. Add/remove components as needed
5. Update metadata (timestamp, version)
6. Validate SBOM format
7. Commit to version control
8. Include in release artifacts

## Validating SBOM

### Basic JSON Validation

#### CycloneDX Format (sbom.json)

```bash
# Validate JSON syntax
jq empty sbom.json

# Validate CycloneDX structure
jq -e '.bomFormat == "CycloneDX"' sbom.json
jq -e '.specVersion' sbom.json
jq -e '.metadata' sbom.json
jq -e '.components' sbom.json
```

#### SPDX Format (sbom.spdx.json)

```bash
# Validate JSON syntax
jq empty sbom.spdx.json

# Validate SPDX structure
jq -e '.spdxVersion == "SPDX-2.3"' sbom.spdx.json
jq -e '.dataLicense' sbom.spdx.json
jq -e '.creationInfo' sbom.spdx.json
jq -e '.packages' sbom.spdx.json
```

### SPDX Validation

```bash
# Install tools
pip install spdx-tools

# Validate SBOM
pyspdxtools -i sbom.spdx.json
```

## SBOM Best Practices

1. **Completeness**: Include all dependencies, including transitive ones
2. **Accuracy**: Verify version numbers and licenses
3. **Automation**: Use tools to generate SBOMs automatically
4. **Version Control**: Track SBOM changes in Git
5. **Distribution**: Include SBOM with releases
6. **Updates**: Keep SBOM current with dependencies
7. **Verification**: Validate SBOM format and content
8. **Documentation**: Document how to generate and use SBOM

## SBOM and Security

### Vulnerability Scanning

Use SBOM to scan for vulnerabilities:

```bash
# Using OWASP Dependency-Check
dependency-check --scan . --format JSON --enableExperimental

# Using Grype
grype sbom:sbom.json

# Using Trivy
trivy sbom sbom.json
```

### Monitoring

- Set up automated scanning in CI/CD
- Subscribe to security advisories for dependencies
- Use tools like GitHub Dependabot
- Review SBOM regularly for outdated components

## CRA Compliance

For EU Cyber Resilience Act compliance:

1. **Include SBOM in Product Documentation**
   - Make SBOM available to users
   - Update SBOM with each release
   - Document SBOM format and location

2. **Vulnerability Management**
   - Use SBOM for vulnerability tracking
   - Document known vulnerabilities
   - Provide remediation guidance

3. **Transparency**
   - Publish SBOM in accessible format
   - Include license information
   - Document component relationships

See [CRA_COMPLIANCE.md](CRA_COMPLIANCE.md) for complete CRA compliance information.

## Resources

- [CycloneDX Specification](https://cyclonedx.org/)
- [SPDX Specification](https://spdx.dev/)
- [NTIA SBOM Minimum Elements](https://www.ntia.gov/sbom)
- [Agentino Documentation](README.md)
- [Agentino Security Policy](SECURITY.md)
- [CRA Compliance](CRA_COMPLIANCE.md)

## Support

For questions about SBOM generation for Agentino:

- Check the [documentation](README.md)
- Open an [issue](https://github.com/ImagingTools/Agentino/issues)
- Review [CRA compliance documentation](CRA_COMPLIANCE.md)

**Last Updated**: February 2026
