#!/usr/bin/env python3
"""build.py — сборка, деплой и упаковка ProcessPinner."""

import argparse, ctypes, json, os, platform, shutil, subprocess, sys
from pathlib import Path

C = {"R": "\033[0m", "G": "\033[92m", "Y": "\033[93m", "B": "\033[91m", "C": "\033[96m"}
P = Path
ROOT = P(__file__).resolve().parent
PRO  = ROOT / "ProcessPinner.pro"
BUILD = ROOT / "build"
DEPLOY = ROOT / "deploy"

def _color():
    if platform.system().lower() == "windows":
        try:
            h = ctypes.windll.kernel32.GetStdHandle(-11)
            m = ctypes.c_ulong()
            ctypes.windll.kernel32.GetConsoleMode(h, ctypes.byref(m))
            return False  # use ctypes
        except Exception:
            pass
    return True  # ANSI

def _win_color(code):
    ctypes.windll.kernel32.SetConsoleTextAttribute(ctypes.windll.kernel32.GetStdHandle(-11), code)

def p(msg, color=None, err=False):
    use_ctypes = _color()
    if color:
        if use_ctypes:
            map_c = {"G": 2, "B": 4, "Y": 6, "C": 3, "R": 7}
            _win_color(map_c.get(color, 7))
            print(msg)
            _win_color(7)
        else:
            tag = {"G": "✓", "B": "✗", "Y": "!", "C": "=", "R": "!"}
            c = C[color]
            print(f"{c}{tag.get(color, '')} {msg}{C['R']}", file=sys.stderr if color == "B" else sys.stdout)
    else:
        print(msg)

def run(cmd, cwd=None, capture=False):
    r = subprocess.run(cmd, cwd=str(cwd) if cwd else None, capture_output=capture, text=True, check=False)
    if r.returncode and not capture:
        p(r.stderr.strip() or "Command failed", "B", err=True)
    return r

def which(name):
    return shutil.which(name)

def find_qmake():
    for n in ("qmake", "qmake6", "qmake-qt6", "qmake5", "qmake-qt5"):
        p2 = which(n)
        if p2: return Path(p2)
    for p in ("/opt/qt6/bin/qmake", "/opt/qt5/bin/qmake",
              "C:/Qt/6.x/msvc/bin/qmake.exe", "C:/Qt/6.x/mingw/bin/qmake.exe",
              "C:/Qt/5.x/msvc/bin/qmake.exe", "C:/Qt/5.x/mingw/bin/qmake.exe"):
        if Path(p).exists(): return Path(p)
    return None

def find_cqtdeployer():
    p2 = which("cqtdeployer")
    if p2: return Path(p2)
    for p in ("C:/Program Files/CQtDeployer/*/CQtDeployer.exe",
              "C:/Qt/Tools/CQtDeployer/bin/cqtdeployer.exe"):
        for f in Path("C:/").rglob(Path(p).name) if "*" in p else []:
            pass  # skip rglob for safety
    # Simple glob fallback
    import glob as g
    for pat in ("C:/Program Files/CQtDeployer/*/CQtDeployer.exe",):
        for f in g.glob(pat):
            return Path(f)
    return None

def get_version():
    m = None
    try:
        for line in PRO.read_text().splitlines():
            if line.strip().startswith("VERSION"):
                m = line.split("=", 1)[1].strip().strip('"')
                break
    except Exception:
        pass
    return m or "0.0.0"

# ── CLI ──
parser = argparse.ArgumentParser(description="ProcessPinner build script")
parser.add_argument("-q", "--qmake", help="Path to qmake")
parser.add_argument("-m", "--make", default="auto", help="Build tool: make/ninja/msbuild/auto")
parser.add_argument("-t", "--threads", type=int, default=0, help="Parallel threads (0=auto)")
parser.add_argument("-b", "--build-dir", help="Build directory")
parser.add_argument("-d", "--deploy-dir", help="Deploy directory")
parser.add_argument("-s", "--stage", choices=["build", "deploy", "package"], help="Stop after stage")
parser.add_argument("-o", "--only-build", action="store_true", help="Only build")
parser.add_argument("-n", "--no-package", action="store_true", help="Skip packaging")
parser.add_argument("-c", "--changelog-only", action="store_true", help="Show changelog only")
parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
parser.add_argument("--compiler", choices=["msvc", "gcc", "clang", "auto"], default="auto", help="Force compiler")
args = parser.parse_args()

# ── Changelog ──
if args.changelog_only:
    cl = ROOT / "changelog"
    if cl.exists():
        print(cl.read_text(encoding="utf-8"))
    sys.exit(0)

# ── Header ──
p(f"ProcessPinner v{get_version()}", "C")

# ── Platform / Compiler ──
plat = platform.system().lower()
if plat == "windows": plat = "windows"
elif plat == "linux": plat = "linux"
elif plat == "darwin": plat = "darwin"
else:
    p(f"Unsupported platform: {plat}", "Y")
    plat = "linux"

compiler = args.compiler
if compiler == "auto":
    if plat == "windows":
        if which("cl"): compiler = "msvc"
        elif which("g++"): compiler = "gcc"
        elif which("clang-cl"): compiler = "clang"
    elif plat == "linux":
        compiler = "gcc" if which("gcc") else "clang"
    else:
        compiler = "clang"

p(f"Platform: {plat} | Compiler: {compiler}", "C")

# ── Toolchain ──
make_tool = args.make
if make_tool == "auto":
    if which("ninja"):
        make_tool = "ninja"
    elif plat == "windows" and compiler == "msvc":
        make_tool = "jom" if which("jom") else "nmake"
    elif plat == "windows" and compiler == "gcc":
        make_tool = "mingw32-make" if which("mingw32-make") else "make"
    else:
        make_tool = "make" if which("make") else "gmake"

p(f"Build tool: {make_tool}", "C")

# ── Directories ──
build_dir = Path(args.build_dir) if args.build_dir else BUILD
deploy_dir = Path(args.deploy_dir) if args.deploy_dir else DEPLOY

# ── Dependencies ──
qmake = Path(args.qmake) if args.qmake else find_qmake()
if not qmake:
    p("qmake not found!", "B", err=True)
    sys.exit(1)
p(f"qmake: {qmake}", "G")

cqt = find_cqtdeployer()
if cqt:
    p(f"CQtDeployer: {cqt}", "G")
else:
    p("CQtDeployer not found — deploy will be manual", "Y")

# ── Build ──
if args.stage not in ("deploy", "package"):
    p("\n=== BUILD ===", "C")
    BUILD.mkdir(parents=True, exist_ok=True)
    p(f"qmake {PRO} CONFIG+=release PREFIX={deploy_dir}", "G")
    r = run([str(qmake), str(PRO), f"CONFIG+=release", f"PREFIX={deploy_dir}"], cwd=BUILD, verbose=args.verbose)
    if r.returncode:
        p("qmake failed", "B", err=True)
        sys.exit(1)

    threads = f"-j{args.threads}" if args.threads else ""
    p(f"Compiling with {make_tool}...", "G")
    r = run([make_tool] + (["-j", str(args.threads)] if args.threads else []), cwd=BUILD)
    if r.returncode:
        p("Compilation failed", "B", err=True)
        sys.exit(1)
    p("Build OK", "G")

    if args.stage == "build" or args.only_build:
        p("Stopping after build", "C")
        sys.exit(0)

# ── Deploy ──
if args.stage != "package" and not args.only_build:
    p("\n=== DEPLOY ===", "C")
    DEPLOY.mkdir(parents=True, exist_ok=True)

    if cqt:
        # Find the built executable
        exe = None
        if plat == "windows":
            for f in (BUILD / "release" / "ProcessPinner.exe", BUILD / "ProcessPinner.exe"):
                if f.exists(): exe = f; break
        elif plat == "linux":
            for f in (BUILD / "ProcessPinner",):
                if f.exists(): exe = f; break

        if exe:
            cmd = [str(cqt), "qt", "win", "gui",
                   "-bin", str(exe),
                   "-dir", str(DEPLOY),
                   "-viewer", "installer"]
            if plat == "linux":
                cmd = [str(cqt), "qt", "linux", "gui",
                       "-bin", str(exe),
                       "-dir", str(DEPLOY),
                       "-viewer", "archive"]
            elif plat == "darwin":
                cmd = [str(cqt), "qt", "mac", "gui",
                       "-bin", str(exe),
                       "-dir", str(DEPLOY),
                       "-bundle", str(DEPLOY / "ProcessPinner.app")]
            p(" ".join(cmd), "G")
            r = run(cmd, cwd=ROOT)
            if r.returncode:
                p("CQtDeployer failed", "B", err=True)
            else:
                p("Deploy OK", "G")
        else:
            p("No executable found to deploy", "Y")
    else:
        p("Skipping deploy (no CQtDeployer)", "Y")

    if args.stage == "deploy":
        p("Stopping after deploy", "C")
        sys.exit(0)

# ── Package ──
if not args.no_package:
    p("\n=== PACKAGE ===", "C")
    ver = get_version()
    out_name = f"ProcessPinner_{ver}_{plat}"

    if plat == "windows":
        out = ROOT / f"{out_name}.7z"
        cqt_dir = DEPLOY
        found_7z = which("7z") or which("7za")
        if found_7z:
            cmd = [found_7z, "a", "-t7z", str(out), ".", "-bd"]
            p(f"7z a {out} ...", "G")
            run(cmd, cwd=cqt_dir)
            p(f"Archive: {out}", "G")
        else:
            p("7-Zip not found — archive not created", "Y")

    elif plat == "linux":
        found_ld = which("linuxdeploy")
        if found_ld:
            # linuxdeploy needs an AppDir
            appdir = DEPLOY / "ProcessPinner.AppDir"
            appdir.mkdir(parents=True, exist_ok=True)
            bin_ = appdir / "ProcessPinner"
            if (DEPLOY / "ProcessPinner").exists():
                shutil.copy2(DEPLOY / "ProcessPinner", bin_)
            cmd = [str(found_ld), "--appdir", str(appdir),
                   "--executable", str(bin_),
                   "--desktop-file", str(ROOT / "resouces" / "ProcessPinner.desktop"),
                   "--icon-file", str(ROOT / "resouces" / "icon.png"),
                   "-e", "qt.conf"]
            p(" ".join(cmd), "G")
            r = run(cmd, cwd=ROOT)
            if r.returncode:
                p("linuxdeploy failed", "B", err=True)
            else:
                img = DEPLOY / "ProcessPinner.AppImage"
                if (appdir / "ProcessPinner").exists():
                    run(["./ProcessPinner", "--appimage-update"], cwd=appdir)
                p("AppImage generated", "G")
        else:
            p("linuxdeploy not found", "Y")

    elif plat == "darwin":
        found_mdqt = which("macdeployqt") or None
        if found_mdqt:
            app = DEPLOY / "ProcessPinner.app"
            if app.exists():
                cmd = [str(found_mdqt), str(app), "-dmg"]
                p(" ".join(cmd), "G")
                run(cmd, cwd=DEPLOY)
                p(f"DMG: {app.parent / app.name.replace('.app', '')}.dmg", "G")
        else:
            p("macdeployqt not found", "Y")

p("\nDone!", "G")
