import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;

/**
 * @author Nikita Zulyaev
 */
public class Main {
    static long parseAddress(String address) {
        return Long.parseLong(address);
//        return Long.parseLong(address.substring(2), 16);
    }

    static BufferedReader in;
    static PrintWriter out;

    static Map<Long, Long> vars = new HashMap<>();
    static Map<Long, Integer> sizes = new HashMap<>();

    static long count = 0;

    static String getVar(long address, boolean create) {
        if (address == 0) {
            System.out.println("ALARM");
            System.exit(1);
        }
        Long index = vars.get(address);
        if (index == null && create) {
            vars.put(address, index = count++);
        }
        return "a[" + index + "]";
    }

    static void test(String oldAddress, long newAddress, int size) {
        sizes.put(newAddress, size);
        out.println("test(" + oldAddress + ", " + getVar(newAddress, false) + ", " + size + ", " + size + ");");
    }

    static void test(String oldAddress, long newAddress, int oldSize, int size) {
        sizes.put(newAddress, size);
        out.println("test(" + oldAddress + ", " + getVar(newAddress, false) + ", " + oldSize + ", " + size + ");");
    }

    static void testAll() {
        for (Map.Entry<Long, Long> e : vars.entrySet()) {
            out.println("test(a[" + e.getValue() + "], " + "a[" + e.getValue() + "], " + sizes.get(e.getKey()) + ", " + sizes.get(e.getKey()) + ");");
        }
    }

    public static void main(String[] args) throws IOException {
        in = new BufferedReader(new FileReader("log.txt"));
        out = new PrintWriter("commands.txt");
        String line;
        while ((line = in.readLine()) != null) {
            StringTokenizer tok = new StringTokenizer(line);
            String cmd = tok.nextToken();
            switch (cmd) {
                case "malloc": {
                    int size = Integer.parseInt(tok.nextToken());
                    long address = parseAddress(tok.nextToken());
                    if (size!=0) {
                        out.println(getVar(address, true) + " = malloc(" + size + ");");
                        test(getVar(address, false), address, size);
                    }
                    break;
                }
                case "free": {
                    long address = parseAddress(tok.nextToken());
                    if (address == 0) {
                        out.println("free(0);");
                    } else {
                        out.println("free(" + getVar(address, false) + "); // 0x " + address);
                        vars.remove(address);
                    }
                    break;
                }
                case "calloc": {
                    int a = Integer.parseInt(tok.nextToken());
                    int b = Integer.parseInt(tok.nextToken());
                    long address = parseAddress(tok.nextToken());
                    out.println(getVar(address, true) + " = calloc(" + a + ", " + b + ");");
                    test(getVar(address, false), address, a * b);
                    break;
                }
                case "realloc": {
                    int size = Integer.parseInt(tok.nextToken());
                    long from = parseAddress(tok.nextToken());
                    long to = parseAddress(tok.nextToken());
                    Integer oldSize = sizes.get(from);
                    if (oldSize == null) {
                        oldSize = 0;
                    }
                    String fstr = from == 0 ? "0" : getVar(from, false);
                    if (to == 0) {
                        System.out.println("ALARM");
                        System.exit(1);
                    }
                    out.println(getVar(to, true) + " = realloc(" + fstr + ", " + size + ");");
                    test(fstr, to, oldSize, size);
                    break;
                }
                case "posix_memalign": {
                    int align = Integer.parseInt(tok.nextToken());
                    int size = Integer.parseInt(tok.nextToken());
                    long address = parseAddress(tok.nextToken());
                    out.println("posix_memalign(&" + getVar(address, true) + ", " + align + ", " + size + ");");
                    test(getVar(address, false), address, size);
                }
            }
        }
        testAll();
        out.close();
    }
}