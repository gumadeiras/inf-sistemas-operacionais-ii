bplist00�_WebMainResource�	
_WebResourceData_WebResourceMIMEType_WebResourceTextEncodingName^WebResourceURL_WebResourceFrameNameOC<html><head></head><body><pre style="word-wrap: break-word; white-space: pre-wrap;">class SimpleThread extends Thread {
	
	private String name;
	
	public SimpleThread(String name) {
		this.name = name;
	}

	public void run() {
		int i = 10;
		while (i &gt; 0) {
			System.out.println(name + " executando...");
			
			try { Thread.currentThread().sleep(1000);}
			catch (InterruptedException ie) { ie.printStackTrace(); }
			
			i--;
		}
	}	
}

public class ThreadTester {
	
	public static void main(String args[]) {
		
		SimpleThread thr1 =  new SimpleThread("Joe");
		SimpleThread thr2 =  new SimpleThread("Paul");
		
		thr1.start();
		thr2.start();
		
		try {
			thr1.join();
			thr2.join();
		} catch (InterruptedException ie) { 
			ie.printStackTrace(); 
		}
		
		System.out.println("Main finalizando...");
	}
}</pre></body></html>Ztext/plainUUTF-8_Yhttps://moodle.inf.ufrgs.br/pluginfile.php/65824/mod_resource/content/2/ThreadTester.javaP    ( : P n } ����H                           I