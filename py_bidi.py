import subprocess, os, sys, subprocess

from bazel_tools.tools.python.runfiles import runfiles

server_binary_name = sys.argv[1].replace("./", "")
server_binary = runfiles.Create().Rlocation( "rpcing/" + server_binary_name )

client_binary_name = sys.argv[2].replace("./", "")
client_binary = runfiles.Create().Rlocation( "rpcing/" + client_binary_name )

server = subprocess.Popen([server_binary, "--connect=", "--listen_ms=2000"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
clientCount = 3

client_procs = []
client_outs = []
client_errs = []

for client in range(clientCount):
    proc = subprocess.Popen([client_binary, "--listen="], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    client_procs.append( proc )
    print( proc )

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
