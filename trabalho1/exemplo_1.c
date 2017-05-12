bplist00�_WebMainResource�	
_WebResourceData_WebResourceMIMEType_WebResourceTextEncodingName^WebResourceURL_WebResourceFrameNameO5<html><head></head><body><pre style="word-wrap: break-word; white-space: pre-wrap;">/* Orion Lawlor's Short UNIX Examples, olawlor@acm.org 2004/9/5 
   Shows how to use fork() in a UNIX program.
*/
#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;
#include &lt;unistd.h&gt;
#include &lt;sys/types.h&gt; /* for pid_t */
#include &lt;sys/wait.h&gt; /* for wait */

void doWork(char *arg) {
    while (1) {
		printf("%s\n", arg);
	}
}

int main()
{
    /*Spawn a new process to run alongside us.*/
    pid_t pid = fork();
    if (pid == 0) {		/* child process */
		doWork("child");  
		exit(0);
    }
    else {			/* pid!=0; parent process */
		//printf("sou o pai e vou acabar logo");
		doWork("parent");
		waitpid(pid,0,0);	/* wait for child to exit */
    }
    return 0;
}
</pre></body></html>Ztext/plainUUTF-8_Shttps://moodle.inf.ufrgs.br/pluginfile.php/65821/mod_resource/content/1/exemplo_1.cP    ( : P n } ����4                           5