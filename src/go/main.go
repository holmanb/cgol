package main

import (
	"fmt"
	"strings"
	"os"
	"encoding/csv"
	"time"
)

type format struct {
	live, dead string
	delim rune
}

const (
	defaultFile = "../../data/default"
)

type Array [][]bool

type Matrix struct {
	primary Array
	secondary Array
	x int
	y int
}

func clearScreen(){
	fmt.Println("\033[H\033[J")
}

func swap(a, b *Array){
	*a, *b = *b, *a
}

func (m *Matrix) ReadFile(file string, f format) {
	csvfile, err := os.Open(file)
	if err != nil {
		panic("Couldn't open the csv file")
	}
	r := csv.NewReader(csvfile)
	r.Comma = f.delim
	csvStr,err := r.ReadAll()
	if err != nil {
		panic(err)
	}
	fmt.Print(csvStr)
	m.x = len(csvStr)
	m.primary = make(Array, m.x)
	m.secondary = make(Array, m.x)
	for i := range csvStr {
		if i == 0 {
			m.y = len(csvStr[i])
		} else if len(csvStr[i]) != m.y {
			panic("error parsing file, jagged array")
		}
		m.primary[i] = make([]bool, len(csvStr[i]))
		m.secondary[i] = make([]bool, len(csvStr[i]))
		for j := range csvStr {
			if csvStr[i][j] == "1" {
				m.primary[i][j] = true
			} else if csvStr[i][j] == "0" {
				m.primary[i][j] = false
			}
		}
	}
}

func (m Matrix) String() string{
	var sb strings.Builder
	for i := range m.primary {
		for j := range m.primary {
			sb.WriteString(" ")
			if m.primary[i][j] {
				sb.WriteString("1")
			} else {
				sb.WriteString(" ")
			}
			sb.WriteString(" ")
		}
		sb.WriteString("\n")
	}
	return sb.String()
}

func (matrix *Matrix) Cgol(){
	var X, Y, count int
	for i,row := range matrix.primary {
		for j := range row {
			// box
			for m := -1; i<2; i++ {
				for n:= -1; j<2; j++ {
					if  i == 0 && m == -1 {
						X = matrix.x - 1
					} else if i == matrix.x - 1 && m == 1 {
						X = 0
					}

					if  j == 0 && n == -1 {
						Y = matrix.y - 1
					} else if j == matrix.y - 1 && n == 1 {
						Y = 0
					}
					if matrix.primary[X][Y]{
						count++
						fmt.Println("yay3")
					}
				}
			}
			if count == 3 {
				matrix.secondary[X][Y] = true
				fmt.Println("yay")
			} else if matrix.primary[X][Y] && count == 2 {
				matrix.secondary[X][Y] = true
				fmt.Println("yay2")
			}
		}
	}
	matrix.primary = matrix.secondary
	/*
	for i := 0; i < matrix.x; i++ {
		for j := 0; j < matrix.y; j++ {
			matrix.secondary[i][j] = false
		}
	}
	*/
}

func main() {
	f := format{delim: ';', live:"1", dead:"0"}
	matrix := Matrix{}
	matrix.ReadFile(defaultFile, f)
	clearScreen()
	fmt.Println(matrix)
	time.Sleep(time.Second)
	clearScreen()
	matrix.Cgol()
	fmt.Println(matrix)
}
