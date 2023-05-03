use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    assert!(args.len() >= 4);
    assert!(args.len() <= 8);
    let nplayers: u8 = args[1].parse::<u8>().unwrap();
    let h1: &String = &args[2];
    let h2: &String = &args[3];
    println!("{nplayers}");
    println!("{h1}");
    println!("{h2}");
}
