package server

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

func StartFileServer() {
	laddr, err := net.ResolveTCPAddr("tcp", ":8888")
	if err != nil {
		log.Fatalln(err)
	}
	listener, err := net.ListenTCP("tcp", laddr)
	if err != nil {
		log.Fatalln(err)
	}

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go handleFileConn(conn)
	}
}

func handleFileConn(conn net.Conn) {
	TCPConn := conn.(*net.TCPConn)
	defer TCPConn.Close()

	f, _ := os.OpenFile("Astesia.png", os.O_RDONLY, 0666)
	defer f.Close()
	bufFile := bufio.NewReader(f)
	buf := make([]byte, 1024)
	for {
		n, err := bufFile.Read(buf)
		TCPConn.Write(buf[:n])
		if err == io.EOF {
			break
		}
	}
	TCPConn.CloseWrite()
	n, _ := TCPConn.Read(buf)
	fmt.Printf("Message: %s\n", buf[:n])
}
