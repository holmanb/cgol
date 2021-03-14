package main

import (
	"testing"
)

var m Matrix

func benchmarkLife(size uint, b *testing.B) {
	matrix := Matrix{}
	matrix.Generate(size, size)
	b.ResetTimer() // don't measure init time
	for i := 0; i < b.N; i++ {
		matrix.Cgol()
	}
	// prevent compiler from optimizing away code we're trying to measure
	m = matrix
}
func BenchmarkLife5(b *testing.B) {
	benchmarkLife(2<<5, b)
}
func BenchmarkLife6(b *testing.B) {
	benchmarkLife(2<<6, b)
}
func BenchmarkLife7(b *testing.B) {
	benchmarkLife(2<<7, b)
}
func BenchmarkLife8(b *testing.B) {
	benchmarkLife(2<<8, b)
}
func BenchmarkLife9(b *testing.B) {
	benchmarkLife(2<<9, b)
}
func BenchmarkLife10(b *testing.B) {
	benchmarkLife(2<<10, b)
}
func BenchmarkLife11(b *testing.B) {
	benchmarkLife(2<<11, b)
}
func BenchmarkLife12(b *testing.B) {
	benchmarkLife(2<<12, b)
}
func BenchmarkLife13(b *testing.B) {
	benchmarkLife(2<<13, b)
}
