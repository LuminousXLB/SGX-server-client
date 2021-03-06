# SGX-server-client

## Introduction

This is a project designed to be an example for SGX program to behave differently according to command line parameters.

There's a server and a client connected with socket. Upon the connection sucessfully built, they agree with a session key with Diffie–Hellman Protocol. After that, the server acts as a **echo server** and repeats whatever the client says. Users can type in any message from keyboard to the client side, and see server's reply on the screen.

## Usage

1. Build
   ```console
   # Build in Hardware Mode
   $ make

   # Build in Simulation Mode
   $ make SGX_MODE=SIM
   ```
   Note: If failed due to SDK location problem, build with this command
   ```console
   # Build by specifiying SDK directory
   $ make SGX_SDK=<path_to_sgxsdk_directory>
   ```
2. Run a server
   ```console
   $ ./app s 5000
   ```
   Note: If failed due to library path issue, execute this command before run the server
   ```console
   $ export LD_LIBRARY_PATH=<path_to_linux-sgx/build/linux>:$LD_LIBRARY_PATH
   ```
3. Run a client
   ```console
   $ ./app c localhost 5000
   ```
   Note: If failed due to library path issue, execute this command before run the client
   ```console
   $ export LD_LIBRARY_PATH=<path_to_linux-sgx/build/linux>:$LD_LIBRARY_PATH
   ```

## Known issues

There's a significant flaw in the program. In the project, client and server agree with a shared key using the simplest version of Diffie-Hellman Key Exchange Protocol. Document suggests that we should always hold dh_msg buffer in enclave space, but we didn't. So it will be trivial for malicious Operating System to conduct a MiTM attack.

## Thanks

To excellent code base from <http://csapp.cs.cmu.edu/3e/code.html>. I used it to build socket server and client.
