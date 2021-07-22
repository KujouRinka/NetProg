package server

import (
	"bufio"
	"log"
	"net"
)

func StartEchoServer() {
	laddr, err := net.ResolveTCPAddr("tcp", ":8888")
	listener, err := net.ListenTCP("tcp", laddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go handleConnFromClient(conn)
	}
}

func handleConnFromClient(conn net.Conn) {
	defer conn.Close()
	scanner := bufio.NewScanner(conn)
	for scanner.Scan() {
		t := scanner.Text() + "\n"
		if t == "Q\n" {
			break
		}
		conn.Write([]byte(t))
	}
}
