package server

import (
	"io"
	"log"
	"net"
	"os"
)

func RunHelloClient() {
	tcpAddr, err := net.ResolveTCPAddr("tcp", "127.0.0.1:8888")
	if err != nil {
		log.Fatalln(err)
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer conn.Close()

	io.Copy(os.Stdout, conn)
}
