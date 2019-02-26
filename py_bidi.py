import subprocess, os, sys, subprocess

from bazel_tools.tools.python.runfiles import runfiles

binary_name = sys.argv[1].replace("./", "")
binary = runfiles.Create().Rlocation( "rpcing/" + binary_name )

server = subprocess.Popen([binary, "--connect", ".", "--listen_secs", "10"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
clientCount = 100

client_procs = []
client_outs = []
client_errs = []

for client in range(clientCount):
    client_procs.append( subprocess.Popen([binary, "--listen", "."], stdout=subprocess.PIPE, stderr=subprocess.PIPE) )

for client in range(clientCount):
    out, err = client_procs[client].communicate()
    client_outs.append(out)
    client_errs.append(err)

out, err = server.communicate()

for client in range(clientCount):
    print("client", client)
    print(client_outs[client])
    print(client_errs[client])

print("server")
print(out)
print(err)
