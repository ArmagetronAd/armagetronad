# docker/deploy/ Directory

## Summary
Deployment scripts and configurations for building and distributing Armagetron Advanced packages.

## Details

The deploy directory contains scripts for building, packaging, and distributing Armagetron Advanced to various platforms and distribution channels. These scripts automate the process of creating releases for different targets including Steam, itch.io, PPA, Flatpak, and more.

The directory includes:
- Platform-specific deployment scripts (Steam, itch.io, PPA, Flatpak, GitLab, etc.)
- Utility scripts for preparing and managing deployments
- Configuration files and secrets management
- Release notes and version information

## Directory Structure

```
.
├── RELEASENOTES.md            # Release notes for deployments
├── anonymous_secrets.b64     # Encoded secrets for anonymous deployments
├── deploy_docker.sh          # Docker-based deployment script
├── deploy_download.sh        # Download deployment script
├── deploy_flatpak.sh         # Flatpak deployment script
├── deploy_gitlab.sh         # GitLab deployment script
├── deploy_itch.sh            # itch.io deployment script
├── deploy_lp.sh              # Launchpad deployment script
├── deploy_ppa.sh             # Personal Package Archive deployment
├── deploy_scp.sh             # SCP-based deployment script
├── deploy_steam.sh           # Steam deployment script
├── deploy_zeroinstall.sh     # Zero Install deployment script
├── lp-project-upload         # Launchpad project upload script
├── obtain_steam_guard_1.sh   # Steam Guard retrieval script
├── obtain_steam_guard_2.sh   # Steam Guard retrieval script (alternative)
├── prepare_deploy.sh         # Deployment preparation script
├── steamcontentbuilder/      # Steam content builder tools
├── targets.sh.in             # Deployment target definitions template
├── update_zeroinstall.sh     # Zero Install update script
└── wait_for_upload.sh        # Upload completion wait script
```

## Technologies

- **Scripting**: Bash shell scripts
- **Platforms**: Steam, itch.io, Launchpad PPA, Flatpak, GitLab, Zero Install
- **Containerization**: Docker (for some deployment methods)

## Coding Conventions

- **Shell Scripts**: POSIX-compliant bash scripts
- **Error Handling**: Check return codes and handle errors gracefully
- **Configuration**: Environment variables and command-line arguments

## Key Patterns

- Automated deployment pipeline
- Multi-platform distribution
- Secrets management for authenticated deployments
- Template-based configuration (targets.sh.in)
