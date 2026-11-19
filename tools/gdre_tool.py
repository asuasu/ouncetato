import hashlib
import os
from pathlib import Path
import platform
import shutil
import sys
import urllib.request
import zipfile

VERSION = "2.7.4"


def download_info():
    if sys.platform == "darwin":
        return (
            "macos",
            "Godot RE Tools.app/Contents/MacOS/Godot RE Tools",
            "a148b9509c2826fcc238e81c21005bf3128bdcee1a87c3f79ad943a205aa2170",
        )
    if sys.platform.startswith("linux"):
        if platform.machine().lower() not in ("x86_64", "amd64"):
            raise RuntimeError("GDRE Tools currently provides Linux binaries only for x86_64")
        return (
            "linux",
            "gdre_tools.x86_64",
            "3dc4f1244dda62e41e0c952ac00a69e60e07f3b0d5a211164806bf9d41a54758",
        )
    if sys.platform in ("win32", "cygwin"):
        return (
            "windows",
            "gdre_tools.exe",
            "af24c563eee501effb194d83595de64696c32547b70b15e72e2546790a6d287d",
        )
    raise RuntimeError(f"Unsupported host platform: {sys.platform}")


def cache_directory():
    override = os.environ.get("BROTATO_GDRE_CACHE")
    if override:
        return Path(override).expanduser()
    return Path.home() / ".cache" / "brotato-switch2" / f"gdre-v{VERSION}"


def cached_tool():
    return cache_directory() / download_info()[1]


def download_tool():
    platform_name, relative, expected_hash = download_info()
    cache = cache_directory()
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / "gdre.zip"
    url = f"https://github.com/GDRETools/gdsdecomp/releases/download/v{VERSION}/GDRE_tools-v{VERSION}-{platform_name}.zip"
    request = urllib.request.Request(url, headers={"User-Agent": "brotato-switch2"})
    print(f"Downloading GDRE Tools v{VERSION}")
    with urllib.request.urlopen(request, timeout=120) as response, archive.open("wb") as output:
        shutil.copyfileobj(response, output, 1024 * 1024)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != expected_hash:
        raise RuntimeError(f"GDRE Tools checksum mismatch: expected {expected_hash}, got {digest}")
    with zipfile.ZipFile(archive) as zipped:
        zipped.extractall(cache)
    tool = cache / relative
    tool.chmod(tool.stat().st_mode | 0o111)
    archive.unlink()
    return tool


def resolve_gdre(explicit=None, project_root=None):
    candidates = []
    if explicit:
        candidates.append(Path(explicit).expanduser())
    for name in ("GDRE", "GDRE_TOOLS"):
        if os.environ.get(name):
            candidates.append(Path(os.environ[name]).expanduser())
    for name in ("gdre_tools", "Godot RE Tools"):
        found = shutil.which(name)
        if found:
            candidates.append(Path(found))
    if project_root:
        project_root = Path(project_root)
        candidates.extend([
            project_root / "work/gdre/Godot RE Tools.app/Contents/MacOS/Godot RE Tools",
            project_root / "work/gdre/gdre_tools.x86_64",
            project_root / "work/gdre/gdre_tools.exe",
        ])
    candidates.append(cached_tool())
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return download_tool()
