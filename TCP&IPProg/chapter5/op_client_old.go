package server

import (
	"bufio"
	"io"
	"log"
	"net"
	"strconv"
)

func StartOpClient() {
	raddr, err := net.ResolveTCPAddr("tcp", ":8888")
	if err != nil {
		log.Fatalln(err)
	}
	conn, err := net.DialTCP("tcp", nil, raddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer conn.Close()
	
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()
	n, _ := strconv.Atoi(scanner.Text())
	conn.Write([]byte(scanner.Text() + "\n"))
	for i := 0; i < n + 1; i++ {
		scanner.Scan()
		num := scanner.Text() + "\n"
		conn.Write([]byte(num))
	}
	io.Copy(os.Stdout, conn)
}
