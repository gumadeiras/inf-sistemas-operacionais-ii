bplist00�_WebMainResource�	
_WebResourceData_WebResourceMIMEType_WebResourceTextEncodingName^WebResourceURL_WebResourceFrameNameOU<html><head></head><body><pre style="word-wrap: break-word; white-space: pre-wrap;">#include &lt;stdio.h&gt;
#include &lt;pthread.h&gt;

int g;

void *do_it_1(void *arg) {
    int i, n = *(int *) arg;
    for(i = 0; i &lt; n; i++)
        g = i;
}

void *do_it_2(void *arg) {
    int i, n = *(int *) arg;
    for(i = 0; i &lt; n; i++)
        printf("%d\n", g);
}

int main( int argc, char **argv) {
    pthread_t th1, th2;
    int n = 10;

    pthread_create(&amp;th1, NULL, do_it_1, &amp;n);
    pthread_create(&amp;th2, NULL, do_it_2, &amp;n);
	
	printf("acabando...\n");
}
</pre></body></html>Ztext/plainUUTF-8_Shttps://moodle.inf.ufrgs.br/pluginfile.php/65822/mod_resource/content/3/exemplo_2.cP    ( : P n } ����T                           U