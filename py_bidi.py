import subprocess, os, sys, subprocess

from bazel_tools.tools.python.runfiles import runfiles

cpp_binary_name = sys.argv[1].replace("./", "")
cpp_binary = runfiles.Create().Rlocation( "rpcing/" + cpp_binary_name )

golang_binary_name = sys.argv[2].replace("./", "")
golang_binary = runfiles.Create().Rlocation( "rpcing/" + golang_binary_name )

#server_binary = cpp_binary
server_binary = golang_binary

client_binary = cpp_binary
#client_binary = golang_binary

server = subprocess.Popen([server_binary, "--connect=", "--listen_ms=1000"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
clientCount = 5

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
