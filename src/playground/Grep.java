package playground;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Scanner;

public class Grep {

	static class InputStreamWrapper extends InputStream{

		InputStream in;

		public InputStreamWrapper(String file) throws IOException{
			in = new FileInputStream(new File(file));
		}

		@Override
		//-1 on EOF
		public int read() throws IOException {
			return in.read();
		}

		public void close() throws IOException {
			in.close();
		}

	}

	public static void main(String[] args) throws Exception{
		//arguments
		String file = "src/playground/test";
		String pattern = "(.*\\W)?apple(\\W.*)?";

		//
		Scanner sc = new Scanner(new InputStreamWrapper(file));
		System.out.println(".begin");
		while( sc.hasNextLine() ){
			String line = sc.nextLine();
			if( line.matches(pattern) )
				System.out.println(line);
		}
		System.out.println(".done");
	}


}
