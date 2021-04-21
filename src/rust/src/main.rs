
/*
fn init_new(size: u8) -> &'static mut [&'static mut [i32]] {// Box<[u8]>{
    //println!("{:?}",x);
    /*
    let x: [Box<[u8]>; size] = [
        for i in 0..size {
            Box::new([0,0,0]);
        }
        //Box::new([1,1,1]),
        //Box::new([0,0,0]),
    ];
    return x
    */
    //let (height, width) = (size, size);
    let height = size;
    let width = size;
    let mut oned = vec![0; (height * width).into()];
    let mut slices: Vec<_> = oned.as_mut_slice().chunks_mut(width.into()).collect();
    let grid = slices.as_mut_slice();
}
*/

fn main() {
    const WIDTH: usize = 3;
    const HEIGHT: usize = 3;//(size, size);
    let mut arr = vec![0; HEIGHT*WIDTH];
    set_vec(&mut arr, WIDTH, 1, 1, 1);
    print_vec(&arr,HEIGHT,WIDTH);
    println!("changing value: {} to 0", set_vec(&mut arr, WIDTH, 1, 1, 0));
    print_vec(&arr,HEIGHT,WIDTH);
}

//fn set_vec(v: &std::vec::Vec<u8>, width: usize, x: usize, y: usize, val: u8) -> u8{
fn set_vec(v: &mut std::vec::Vec<u8>, width: usize, x: usize, y: usize, val: u8) -> u8{
    let old = get_vec(v.to_vec(), width, x, y);
    v[width*x + y] = val;
    old
}
fn get_vec(v: Vec<u8>, width: usize, x: usize, y: usize) -> u8{
    v[width*x + y]
}
fn print_vec(v: &Vec<u8>, height: usize, width: usize){
    for i in 0..width {
        for j in 0..height{
           print!("{} ", v[width*i + j]); 
        }
        println!();
    }
}