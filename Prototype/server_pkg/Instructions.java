



package server_pkg;

public class Instructions {
	//public String Instruction_set[] = {"Read(0)", "Write(0, 5)", "CAS(0, 5, 7)"};
	public int no_of_instructions=3;
	public String Instruction_set[] = {"Read", "Write", "CAS"};
	public int first_args[] = {0, 0, 0};
	public byte second_args[] = {0, 5, 5};
	public byte third_args[] = {0, 0, 7};
		
		/*R(Addr); //-> reurns one word - Int
		W(Addr, Int); //-> Int // returns old value(word)
		CAS(Addr, Int, Int);// -> Int // compares Mem[Addr] to 2nd arg; if equal,
		//swaps 3rd arg in; returns old val*/
}
