import os
import sys
import subprocess
import io
import time
from pathlib import Path

dev = "/dev/disk3s2"
idx = "0"
folder = "/Users/matthew"
out = "/Volumes/Backup7/Mac1013"
exclude = ["/Users/matthew/Library/Developer/Xcode/DerivedData",
            "/Users/matthew/Library/Application Support/Google/Chrome"]
paths = []

# stats
last_ts = 0.0
list_errors = 0
list_count = 0
recover_errors = 0

def ls(path):
    global list_errors, list_count
    dirs = []
    files = []
    args = ["./bin/apfs-list", dev, idx, path]
    p = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    list_count = list_count + 1
    if p.returncode == 0:
        lines = p.stderr.decode("utf-8")
        for ln in lines.split("\n"):
            if ln.find("- DIR REC") >= 0:
                parts = ln.split("||")
                if len(parts) >= 4:
                    name = parts[-1].strip().split("=")[-1].strip()
                    if parts[1].strip() == "Dirctry":
                        dirs.append(name)
                    else:
                        files.append(name)
    else:
        list_errors = list_errors+1
        print("\n" + " ".join(args))
    return [dirs, files]

def scan(path):

    global total, last_ts, paths, list_errors, list_count
    for e in exclude:
        if e == path:
            return 0

    dirs,files = ls(path)
    for f in files:
        paths.append(path + "/" + f)
    if time.time() - last_ts > 2.0:
        print("Scanning", len(paths), "files...", "(%i errors %.1g%%)" % (list_errors, list_errors*100.0/list_count))
        last_ts = time.time()

    # and then scan the subfolders...
    for d in dirs:
        s = path + "/" + d
        scan(s)

scan(folder)

if 0:
    start_ts = time.time()

    for progress, i in enumerate(paths):
        fld,leaf = os.path.split(i)
        part = fld[len(folder):]

        # check if we need to create output folder?
        outdir = out + leaf
        if not os.path.isdir(outdir):
            os.makedirs(outdir)

        if not leaf == ".DS_Store":
            o = out + part + "/" + leaf
            # print(i, "->", o)
            if not os.path.exists(o) or Path(o).stat().st_size == 0:
                args = ["./bin/apfs-recover", dev, idx, i]
                outfile = open(o, "wb")
                p = subprocess.run(args, stdout=outfile, stderr=subprocess.PIPE)
                if not p.returncode == 0:
                    recover_errors = recover_errors + 1
                outfile.close()
            if time.time() - last_ts > 2.0:
                last_ts = time.time()
                elapsed = last_ts - start_ts
                if elapsed > 0.0:
                    rate = progress / elapsed
                    if rate > 0:
                        remaining_files = len(paths) - progress
                        remaining_time = int(remaining_files / rate)
                        hrs = remaining_time / 3600
                        remaining_time = remaining_time - (hrs * 3600)
                        min = remaining_time / 60
                        sec = remaining_time % 60
                        print("Exporting", progress, "of", len(paths), "(%.2f%%)" % (progress * 100.0 / len(paths)), "files...", "%ih%im%is" % (hrs, min, sec))

