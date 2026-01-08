UpdateManager (Orchestrator)
 ├─ ManifestLoader → Manifest (artifacts + boot strategy)
 ├─ Planner → InstallPlan (ordered steps)
 ├─ Verifier:
 │    ├─ HashVerifier (SHA-256)
 │    └─ SignatureVerifier (X.509 CMS/PKCS#7)
 ├─ ArtifactHandlers (polymorphic):
 │    ├─ RawImageHandler           [.img, .bin, .img.gz/.zst]
 │    ├─ Ext4ImageHandler          [.img] (raw write + optional resize)
 │    ├─ TarArchiveHandler         [.tar, .tar.gz, .tar.zst]
 ├─ Stream Pipeline:
 │    ├─ Reader (FileReader/HttpReader/…)
 │    ├─ Decompressor (Gzip/Zstd/None)
 │    └─ Writer (BlockDeviceWriter/FileSystemWriter)
 ├─ SlotManager (A/B)
 ├─ BootControl (U‑Boot/EFI/systemd-boot)
 ├─ Journal (UpdateState, resume)
 ├─ HealthCheck (post-install validation)
 ├─ Progress & Logger



## Key principles

* Streaming (no full buffers): Reader → Decompressor → Tee(Hash) → Writer
* Atomicity: install to inactive slot, validate, then switch boot target.
* Resilience: journal each step; allow re-entry after reboot.
