import os
import sys
import subprocess
import io
import time

dev = "/dev/disk2s2"
idx = "0"
folder = "/Users/matthew"
out = "/Volumes/Backup7/Mac1013"
exclude = ["/Users/matthew/Library/Developer/Xcode/DerivedData",
            "/Users/matthew/Library/Application Support/Google/Chrome"]
paths = []

# stats
last_ts = 0.0

def ls(path):
    dirs = []
    files = []
    args = ["./bin/apfs-list", dev, idx, path]
    p = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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
    return [dirs, files]

def scan(path):

    global total, last_ts, paths
    for e in exclude:
        if e == path:
            return 0

    dirs,files = ls(path)
    for f in files:
        paths.append(path + "/" + f)
    if time.time() - last_ts > 2.0:
        print("Scanning", len(paths), "files...")
        last_ts = time.time()

    # create output folder
    leaf = path[len(folder):]
    outdir = out + leaf
    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    # and then scan the subfolders...
    for d in dirs:
        s = path + "/" + d
        scan(s)

scan(folder)

if 1:
    start_ts = time.time()

    for progress, i in enumerate(paths):
        fld,leaf = os.path.split(i)
        part = fld[len(folder):]
        if not leaf == ".DS_Store":
            o = out + part + "/" + leaf
            # print(i, "->", o)
            if not os.path.exists(o):
                args = ["./bin/apfs-recover", dev, idx, i]
                outfile = open(o, "wb")
                p = subprocess.run(args, stdout=outfile, stderr=subprocess.PIPE)
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
                        remaining_time -= hrs * 3600
                        min = remaining_time / 60
                        remaining_time -= min * 60
                        sec = remaining_time % 60
                        print("Exporting", progress, "of", len(paths), "(%.2f%%)" % (progress * 100.0 / len(paths)), "files...", "%ih%im%is" % (hrs, min, sec))

