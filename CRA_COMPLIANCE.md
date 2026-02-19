# EU Cyber Resilience Act (CRA) Compliance

## Overview

This document describes how the Agentino project complies with the EU Cyber Resilience Act (Regulation (EU) 2024/2847).

## Product Information

**Product Name**: Agentino - Distributed Service Management System  
**Product Type**: Commercial software application/framework  
**License**: Commercial (LicenseRef-Agentino-Commercial)  
**Manufacturer**: ImagingTools GmbH  
**Product Category**: Distributed system management software

## CRA Requirements Compliance

### 1. Security Requirements (Article 10)

#### 1.1 Secure by Design

Agentino follows secure development practices:

- **Input Validation**: All GraphQL inputs and service parameters include validation mechanisms
- **Type Safety**: Strongly typed interfaces using C++17 and Qt framework features
- **Memory Safety**: Uses Qt framework's memory management and modern C++ smart pointers
- **Authentication**: Built-in authentication mechanisms via ImtCore framework
- **Code Review**: Changes undergo review process before integration
- **Secure Communication**: WebSocket-based communication with encryption support

#### 1.2 Security Updates

- Security updates are provided for supported versions
- Critical vulnerabilities are addressed with priority
- Security advisories are published through GitHub Security Advisories
- See [SECURITY.md](SECURITY.md) for our security update policy

#### 1.3 Vulnerability Handling

- Documented process for reporting vulnerabilities (see SECURITY.md)
- Response timeline: acknowledgment within 72 hours
- Resolution target: within 90 days based on severity
- Coordinated disclosure with security researchers

### 2. Vulnerability Management (Article 11)

#### 2.1 Identification

- Regular dependency monitoring
- Security scanning in development workflow
- Community-driven vulnerability reports
- Automated security checks via GitHub tools

#### 2.2 Documentation

- Known vulnerabilities documented in security advisories
- CVE identifiers assigned for critical issues
- Mitigation guidance provided in advisories
- Fixes tracked in release notes

#### 2.3 Remediation

- Security patches released as updates
- Hotfixes for critical vulnerabilities
- Backports to supported versions when feasible
- Migration guidance for deprecated features

### 3. Transparency Requirements (Article 13)

#### 3.1 Product Documentation

- Comprehensive documentation in README.md
- Architecture and component descriptions
- Build and installation instructions
- Usage examples and getting started guide

#### 3.2 Security Information

- Security policy: [SECURITY.md](SECURITY.md)
- Supported versions clearly documented
- Known limitations and security considerations
- Safe usage guidelines

#### 3.3 Software Bill of Materials (SBOM)

Agentino's main dependencies:

**Runtime Dependencies:**
- Qt Framework (version 6.x) - Qt Project (Commercial license required)
- ImtCore Framework - Enterprise application framework (proprietary)
- ACF (Application Component Framework) - ImagingTools GmbH (Commercial)
- OpenSSL - Security and encryption
- Quazip - ZIP compression/decompression (LGPL-2.1)
- Standard C++ library (implementation-dependent)

**Build Dependencies:**
- CMake 3.26 or later (BSD License)
- QMake (part of Qt)
- Doxygen (GPL) - for documentation generation

SBOM can be generated using standard tools:
- For CMake builds: Use `cmake --graphviz` or SBOM generation tools
- For package distributions: Use platform-specific SBOM tools

See [SBOM.md](SBOM.md) for detailed SBOM information.

#### 3.4 Commercial Licensing Compliance

- Source code available under commercial license
- License: Commercial (see Install/Commercial/)
- Dependencies are documented in README.md
- Build from source available to licensed customers

### 4. Support and Maintenance (Article 14)

#### 4.1 Support Period

- **Current Release**: Full support including security updates
- **Previous Release**: Security updates for critical issues
- **Older Releases**: Community support, no official security updates

#### 4.2 End of Support

- When a version reaches end of support, it will be announced in:
  - GitHub releases
  - Repository README
  - Project documentation
  - Minimum 6 months notice for planned end of support

#### 4.3 Maintenance Activities

- Bug fixes and security patches
- Compatibility updates for dependencies
- Documentation improvements
- Performance optimizations

### 5. Conformity Assessment

#### 5.1 Self-Assessment

As a commercial distributed service management system:

- Agentino provides infrastructure for managing distributed services
- Deployment operators are responsible for their own CRA compliance
- Agentino maintains documentation to support downstream compliance
- Security practices align with CRA requirements

#### 5.2 Third-Party Dependencies

Users must ensure compliance for:

- Qt Framework (from Qt Project)
- ImtCore Framework (proprietary - from ImagingTools GmbH)
- ACF Framework (from ImagingTools GmbH)
- OpenSSL library
- Platform-specific system libraries
- Any additional dependencies added by downstream users

#### 5.3 Risk Assessment

Agentino is designed as a distributed management system with:

- **Moderate inherent risk**: Manages service lifecycles and network communications
- **Network security**: Requires proper network configuration and access controls
- **User responsibility**: Administrators must implement appropriate security controls
- **Deployment considerations**: Security depends on proper configuration and deployment

### 6. Incident Response

#### 6.1 Security Incident Handling

In case of a security incident:

1. **Detection**: Through reports or automated scanning
2. **Assessment**: Evaluate severity and impact
3. **Communication**: Notify affected users via GitHub advisories
4. **Remediation**: Develop and release fix
5. **Disclosure**: Publish advisory after fix is available

#### 6.2 Communication Channels

- GitHub Security Advisories
- Release notes
- Project documentation updates
- Direct notification to known affected users when possible

### 7. Documentation and Records

#### 7.1 Available Documentation

- Project documentation: [README.md](README.md)
- Security policy: [SECURITY.md](SECURITY.md)
- License information: [Install/Commercial/](Install/Commercial/)
- Build instructions in README.md
- Architecture documentation in README.md

#### 7.2 Change Management

- Version control via Git
- Release notes for each version
- Change tracking in commit history
- Security-relevant changes highlighted

### 8. User Responsibilities

When using Agentino, administrators and operators are responsible for:

#### 8.1 System Security

- Implementing proper network security and firewalls
- Configuring authentication and authorization
- Securing communication channels
- Monitoring system activity and logs
- Following secure deployment practices

#### 8.2 Dependency Management

- Keeping Agentino, Qt, and ImtCore updated
- Monitoring security advisories
- Testing updates before deployment
- Managing their own SBOM

#### 8.3 Compliance

- Ensuring their deployments comply with CRA
- Documenting their use of Agentino in their SBOM
- Implementing their own security measures
- Maintaining their own support processes

### 9. Limitations and Disclaimers

#### 9.1 No Warranty

As stated in the LGPL-2.1 license, Agentino is provided "AS IS" without warranty. See [Install/LGPL/](Install/LGPL/) for full terms.

#### 9.2 Security Limitations

- Agentino provides tools; security depends on correct deployment and configuration
- No system can prevent all security issues
- Users must implement deployment-specific security
- Regular updates are essential for security

#### 9.3 Liability

Liability limitations are defined in the LGPL-2.1 license.

### 10. Contact Information

**Security Issues**: See [SECURITY.md](SECURITY.md) for reporting procedures  
**General Questions**: GitHub Issues  
**Project Repository**: https://github.com/ImagingTools/Agentino  
**Documentation**: [README.md](README.md)

### 11. Updates to This Document

This CRA compliance document is reviewed and updated regularly to reflect:

- Changes in CRA requirements or guidance
- Updates to the project's security practices
- New features or capabilities
- Lessons learned from security incidents

**Version**: 1.0  
**Last Updated**: February 2026  
**Next Review**: August 2026

## References

- [EU Cyber Resilience Act - Regulation (EU) 2024/2847](https://eur-lex.europa.eu/)
- [Agentino Security Policy](SECURITY.md)
- [Agentino Documentation](README.md)
- [GNU LGPL v2.1 License](Install/LGPL/)
- [NIST Secure Software Development Framework (SSDF)](https://csrc.nist.gov/Projects/ssdf)

## Certification Status

**Note**: As an open-source distributed management system, Agentino is designed to support CRA compliance but does not itself undergo certification. Deployments incorporating Agentino must undergo their own conformity assessment as required by the CRA.
