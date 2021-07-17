package server

import (
	"log"
	"net"
)

func RunHelloServer() {
	tcpAddr, err := net.ResolveTCPAddr("tcp", "127.0.0.1:8888")
	if err != nil {
		log.Fatalln(err)
	}
	listener, err := net.ListenTCP("tcp", tcpAddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer listener.Close()

	conn, err := listener.Accept()
	if err != nil {
		log.Fatalln(err)
	}
	defer conn.Close()
	conn.Write([]byte("Hello World"))
}
