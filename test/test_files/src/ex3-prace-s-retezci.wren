// Program 3: Prace s retezci a vestavenymi funkcemi
import "ifj25" for Ifj
class Program {
    static main() {
        var str1 
        str1 = "Toto je text v programu jazyka IFJ25"
        var str2
        str2 = ", ktery jeste trochu obohatime"
        str2 = str1 + (str2 * 3)
        __d = Ifj. write(str1)
        __d =  Ifj . write("\n")
        __d = Ifj .write(str2)
        __d = Ifj.write("\n")
        __d = Ifj.write("Zadejte serazenou posloupnost malych pismen a-h:\n")
        var input
        input = Ifj.read_str()
        var all
        all = ""
        while (input != null) {
            var strcmpResult 
            strcmpResult = Ifj.strcmp(input, "abcdefgh")
            if (strcmpResult == 0) {
                __d = Ifj.write("Spravne zadano! Predchozi pokusy:\n")
                all = all + "\n"
                __d = Ifj.write(all)  // vypsat spojene nepodarene vstupy
                input = null          // misto break
            } else {
                __d = Ifj.write("Spatne zadana posloupnost, zkuste znovu:\n")
                all = all + input // spojuji neplatne vstupy
                input = Ifj.read_str()
            }
        }
    }
}
