# chatserver
chat cluster-server that worked in nginx tcp load-balanced environment and chat client based on muduo and redis library

compile method:
cd build
rm -rf *
cmake ..
make

need to download and configure load-balance module from nginx, muduo network library, redis library, and mysql library.
