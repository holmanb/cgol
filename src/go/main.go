package main

import (
	"encoding/csv"
	"fmt"
	"os"
	"sync"
	"strings"
	"time"
)

type format struct {
	live, dead string
	delim      rune
}

const (
	defaultFile = "../../data/default"
)

type Array [][]bool

type Matrix struct {
	primary   Array
	secondary Array
	x         int
	y         int
}

func clearScreen() {
	fmt.Println("\033[H\033[J")
}

func (m *Matrix) ReadFile(file string, f format) {
	csvfile, err := os.Open(file)
	if err != nil {
		panic("Couldn't open the csv file")
	}
	r := csv.NewReader(csvfile)
	r.Comma = f.delim
	csvStr, err := r.ReadAll()
	if err != nil {
		panic(err)
	}
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

func (m Matrix) String() string {
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

func (matrix *Matrix) Cgol() {
	var waitgroup sync.WaitGroup
	for I, Row := range matrix.primary {
		waitgroup.Add(1)
		go func(i int, row []bool, matrix * Matrix, wg * sync.WaitGroup){
			defer wg.Done()
			for j := range row {
				var X, Y, count, m, n int
				// box
				count = 0
				for m = -1; m < 2; m++ {
					for n = -1; n < 2; n++ {
						if m == 0 && n == 0 {
							continue
						}
						if i == 0 && m == -1 {
							X = matrix.x - 1
						} else if i == matrix.x-1 && m == 1 {
							X = 0
						} else {
							X = i + m
						}

						if j == 0 && n == -1 {
							Y = matrix.y - 1
						} else if j == matrix.y-1 && n == 1 {
							Y = 0
						} else {
							Y = j + n
						}
						if matrix.primary[X][Y] {
							count++
						}
					}
				}
				if count == 3 {
					matrix.secondary[i][j] = true
				} else if matrix.primary[i][j] && count == 2 {
					matrix.secondary[i][j] = true
				}
			}
		}(I, Row, matrix, &waitgroup)
	}
	waitgroup.Wait()
	matrix.primary, matrix.secondary = matrix.secondary, matrix.primary
	/* todo - clearing this matrix can be parallel, but need a way to 
	guarantee it doesn't get used prior to completion */
	for i := 0; i < matrix.x; i++ {
		for j := 0; j < matrix.y; j++ {
			matrix.secondary[i][j] = false
		}
	}
}

func main() {
	f := format{delim: ';', live: "1", dead: "0"}
	matrix := Matrix{}
	matrix.ReadFile(defaultFile, f)
	for {
		clearScreen()
		fmt.Println(matrix)
		matrix.Cgol()
		time.Sleep(time.Millisecond * 500)
	}
}
