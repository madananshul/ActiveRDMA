



package client_pkg;

//import server_pkg.*;

public class Instructions extends Instructions_interface_class{
	//public String Instruction_set[] = {"Read(0)", "Write(0, 5)", "CAS(0, 5, 7)"};
	public int no_of_instructions=3;
	public String Instruction_set[] = {"Read", "Write", "CAS"};
	public int first_args[] = {0, 0, 0};
	public byte second_args[] = {0, 5, 5};
	public byte third_args[] = {0, 0, 7};
	
	public int get_no_of_instructions(){
		return no_of_instructions;
	}
	public String[] get_Instruction_set(){
		return Instruction_set;
	}
	public int[] get_first_args(){
		return first_args;
	}
	public byte[] get_second_args(){
		return second_args;
	}
	public byte[] get_third_args(){
		return third_args;
	}
}
