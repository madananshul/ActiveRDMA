package results_analysis;

import java.util.*;
import java.io.*;

/*
# jvm_nsec insns pkts mem_rd mem_wr mem_cas
273049837 9877994 8336 116566 4815412 2073
# jvm_nsec insns pkts mem_rd mem_wr mem_cas
278899750 9877994 8340 125802 4815412 2073
 */

public class MergeResults {
	
    //String input = "1 fish 2 fish red fish blue fish";
    
    public static void main(String[] args)
    {
    	//long i = 278899750;
    	        
    	//String currentDir = new File(".").getAbsolutePath();
    	//System.out.println(currentDir);
    	String dir = null;
    	MergeResults m = new MergeResults();
    	
    	
    	dir = "./results/arfs/active/ab/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/active_find/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/active_grep/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/find/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/grep/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/scale/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/stream_read/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/active/stream_write/";
    	m.parseFile(dir);
    	
    	//RDMA
    	
    	dir = "./results/arfs/rdma/ab/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/active_find/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/active_grep/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/find/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/grep/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/scale/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/stream_read/";
    	m.parseFile(dir);
    	
    	dir = "./results/arfs/rdma/stream_write/";
    	m.parseFile(dir);
    	
    	//NFS
    	
    	dir = "./results/nfs/ab/";
    	m.parseFile(dir);
    	
    	dir = "./results/nfs/find/";
    	m.parseFile(dir);
    	
    	dir = "./results/nfs/grep/";
    	m.parseFile(dir);
    	
    	dir = "./results/nfs/scale/5/";
    	m.parseFile(dir);
    	
    	dir = "./results/nfs/stream_read/";
    	m.parseFile(dir);
    	
    	dir = "./results/nfs/stream_write/";
    	m.parseFile(dir);
    }
    
    public void parseFile(String dir){
		long before[][] = new long[10][6];
		long after[][] = new long[10][6];
		long delta[][] = new long[10][6];
		double mean[] = {0, 0, 0, 0, 0, 0};
		double std_dev[] = new double[6];
		double std_error_mean[] = new double[6];
		String CI_95[] = new String[6];
		
    	int file = 0;
    	while(file<10){
    		try{

    			File f = new File(dir+"t"+file);
    			Scanner sc = new Scanner(f);
    			//System.out.println(f.getName());
    			//System.out.println(f.getPath());
    			boolean is_before = true;


    			while(sc.hasNextLine()){
    				String line = sc.nextLine();
    				if(line.charAt(0) == '#'){
    					//System.out.println("Skipping line");
    					continue;
    				}
    				//System.out.println("Parsing line");
    				if(is_before){
    					is_before = false;
    					String  b[] = line.split(" ");
    					for (int i = 0; i < b.length; i++) {  
    						before[file][i] = Long.valueOf(b[i]).longValue();
    					}
    				}
    				else {
    					String  b[] = line.split(" ");
    					for (int i = 0; i < b.length; i++) {  
    						after[file][i] = Long.valueOf(b[i]).longValue(); 
    						delta[file][i] = after[file][i]-before[file][i];
    						System.out.print(delta[file][i]+" ");
    						mean[i] += delta[file][i];
    					}
    				}
    				System.out.println("");
    			}
    			
    			
    			
    			sc.close();
    		}
    		catch (FileNotFoundException e) {
    			e.printStackTrace();
    		}
    		file++;
    	}
    	
    	//Write merged results to t10
    	
        try{ 
            FileWriter fstream = new FileWriter(dir+"t10");
            BufferedWriter out = new BufferedWriter(fstream);
            System.out.print("# jvm_nsec insns pkts mem_rd mem_wr mem_cas\n");
            out.write("# jvm_nsec insns pkts mem_rd mem_wr mem_cas\n");
            

			
			for(int i=0;i<6;i++){
            	mean[i]/=10;
            	double sum_squares_deviations = 0;
            	for(int j=0;j<10;j++){
            		sum_squares_deviations += (delta[j][i]-mean[i])*(delta[j][i]-mean[i]);
            		
            	}
            	std_dev[i]  = Math.sqrt(sum_squares_deviations/9);

            	std_error_mean[i] = std_dev[i]/Math.sqrt(10);
            	CI_95[i] = mean[i] + " +- " + (1.96*std_error_mean[i]);
            	out.write(CI_95[i] + " ");
            }
			
            System.out.println();
            out.write("\n");
            out.close();
       }catch (Exception e){//Catch exception if any
              System.err.println("Error: " + e.getMessage());
       }
    }
    
    
    
}
