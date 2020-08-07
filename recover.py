import os
import sys
import subprocess
import io

dev = "/dev/disk2s2"
idx = "0"
folder = "/Users/matthew"
out = "/Volumes/Backup7/Mac1013"

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
                name = ln[55:]
                if parts[1].strip() == "Dirctry":
                    dirs.append(name)
                else:
                    files.append(name)
    return [dirs, files]

def scan(path):
    print(path)
    dirs,files = ls(path)
    leaf = path[len(folder):]
    outdir = out + leaf
    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    # backup files in this folder...
    for f in files:
        if not f == ".DS_Store":
            i = path + "/" + f
            o = out + leaf + "/" + f
            args = ["./bin/apfs-recover", dev, idx, i]

            p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            mem,err = p.communicate()
            nonz = len(mem)
            while nonz > 0 and mem[nonz-1] == 0:
                nonz = nonz - 1
            print(i, str(len(mem))+"->"+str(nonz))
            mem = mem[0:nonz]

            outfile = open(o, "wb")
            outfile.seek(0)
            outfile.truncate()
            outfile.write(mem)
            outfile.close()
            # sys.exit(-1)

    # and then scan the subfolders...
    for d in dirs:
        s = path + "/" + d
        scan(s)

scan(folder)