extern crate csv;

use std::fs::File;

#[allow(dead_code)]

fn main() {
    let mut a = Arr2D::new(3);
    a.print();
    println!("changing value: {} to 1", a.set(1, 1, 1));
    println!();
    a.print();
}

struct Arr2D {
    arr: std::vec::Vec<u8>,
    width: usize,
}

impl Arr2D {
    fn new(size: usize) -> Arr2D {
        Arr2D{
            width: size,
            arr: vec![0; size*size],
        }
    }

    fn load(file: String){
        let file = File::open(file).expect("Unable to open");
        let mut rdr = csv::Reader::from_reader(file);
        for result in rdr.records() {
            let record = result;
            println!("{:?}", record);
        }
    }

    fn get(&self, x: usize, y: usize) -> u8{
        self.arr[self.width*x + y]
    }

    // return old value before setting
    fn set(&mut self, x: usize, y: usize, val: u8) -> u8{
        let old = self.get(x, y);
        self.arr[self.width * x + y] = val;
        old
    }

    fn print(&self){
        for i in 0..self.width {
            for j in 0..self.width{
                print!("{} ", self.arr[self.width*i + j]);
            }
            println!();
        }
    }
}