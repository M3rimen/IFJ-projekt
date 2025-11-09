// Program 1: Vypocet faktorialu (iterativne, bez overeni celociselnosti)
import "ifj25" for Ifj
class Program {
    // Hlavni telo programu - funkce main
    static main() {
        // v zakladnim zadani neumime volat funkci mimo prirazeni, proto zde prirazujeme
        // navratovou hodnotu (kazda funkce nejakou ma) napr. do "dummy" promenne dm
        var dm 
        dm = Ifj.write("Zadejte cislo pro vypocet faktorialu\n")
        var a 
        a = Ifj.read_num()
        if (a != null) {
            if (a < 0) {
                dm = Ifj.write("Faktorial ")
                dm = Ifj.write(a)
                dm = Ifj.write(" nelze spocitat\n")
            } else {
                var vysl 
                vysl = 1
                while (a > 0) {
                    vysl = vysl * a
                    a = a - 1
                }
                vysl = Ifj.floor(vysl)
                vysl = Ifj.str(vysl)
                vysl = "Vysledek: " + vysl + "\n"
                dm = Ifj.write(vysl)
            }
        } else { // a == null
            dm = Ifj.write("Faktorial pro null nelze spocitat\n")
        }
    }
}
