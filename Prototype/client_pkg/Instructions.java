

/*
 * 
 * R(Addr) -> Int
W(Addr, Int) -> Int // returns old value
CAS(Addr, Int, Int) -> Int // compares Mem[Addr] to 2nd arg; if equal,
swaps 3rd arg in; returns old val
RUN(Addr, Int) -> Void // loads class at Address, runs invoke() as defined below

The signature for a class is:

class <name_doesnt_matter> {

public static void invoke(int arg, int[] memory);

}

* Execute(Insn[]) -> Results, Bool[] // execute operations; results is
an array of the return values from each instruction (specified above),
and success/failure bools for each insn

Class is set of insructions(array) including RUN, Rd, Wr etc.

I think we need to provide library that translates higher level 
language to our instruction set.

Our instruction set is loaded into Text memory, and Our instructions directly work on
VIRTUAL MEMORY.

Difference between mobile code and DMA. DMA didnot have any concept of text segment because IO
peripheral was directly accessing just RAM thru data bus, and no program code had to be loaded
into server memory. In our case, we would have to load mobile code into Text segment of 
memory, pin that memory and mobile code program can only manipulate data memory segment(??).
But finally this would have to be done on NIC h/w, so we need not worry about text segment,
hep segment etc.

DMA directly controls physical memory transfers like reads, writes etc.
Thus Memory segment array for prototype is assumed to be the Virtual memory, but we are not 
concerned with the virtual memory to physical memory translation for the prototype.

4 memory types - Text(code), stack, heap segments

In particular, zero-copy RDMA protocols require that the memory pages involved in a 
transaction be pinned, at least for the duration of the transfer. If this is not done, 
RDMA pages might be paged out to disk and replaced with other data by the operating system, 
causing the DMA engine (which knows nothing of the virtual memory system maintained by the 
operating system) to send the wrong data

Stage 1:
Primitive ops like Rd, Wr etc. on Server.memory
Stage 2 :
Primtive ops on memory(a virtual identifier of server memory), which is translated into 
Server.memory at server side.
Stage 3:
Server somehow needs todo validation of instruction format of client sent instructions
 */

//Define primitive operations like read, write, CAS in separate library class file
//Assuming byte addressable memory for now, so word size is a byte

package client_pkg;

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
