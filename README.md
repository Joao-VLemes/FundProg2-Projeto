![LOGO](https://github.com/Joao-VLemes/FundProg2-Projeto/blob/main/Jogo/sources/logojogo.png)



Projeto final da disciplina de **Fundamentos de Programação 2**.
Basicamente é um jogo estilo "Wordle" ou "Loldle", mas com jogos de videogame. Foi feito em **C** usando a biblioteca **Raylib**.

### 👥 Grupo
* João Victor Lemes Cardoso
* Felippe Henrique Teixeira Pedroso
* Bryan Esteves Santana
* **Professor:** Muriel de Souza Godoi

---

## ⚙️ Como funciona

O projeto tem duas partes principais:

1. **Menu no Terminal:** Assim que você roda, abre um menu texto. Ali você pode adicionar, remover ou editar os jogos que ficam salvos no arquivo `list.csv` e `frases.csv`.
2. **O Jogo (Gráfico):** Quando você escolhe a opção "0" no terminal, ele abre a janela do jogo de verdade.

### Como rodar
É só abrir o terminal na pasta `Jogo` e rodar o makefile.
**Linux ou Windows (com MinGW configurado):**
```make```
```make run```

### Como jogar

O objetivo é descobrir qual é o jogo secreto.
1. Clique em Play.
2. Comece a digitar o nome de um jogo e dê ENTER.
3. O jogo vai te dar dicas baseadas nas cores:

- 🟩 Verde: Acertou essa característica (Ex: é da mesma empresa).

- 🟨 Amarelo: É quase isso (Ex: O jogo está na plataforma certa, mas tem outras também).

- 🟥 Vermelho: Errado.

- ⬆️⬇️ Setas no Ano: Diz se o jogo secreto é mais novo ou mais antigo que o seu chute.

Dicas Extras: Conforme você gasta suas vidas (corações), o jogo libera ajudas:

- Uma frase sobre o jogo.
- A capa do jogo borrada.
- A capa nítida.

### ⚠️ Observações

- Os dados dos jogos ficam no list.csv.
- Se você adicionar um jogo novo pelo terminal, ele vai funcionar, mas imagens de capa e logo vão ser criadas vazios, apenas troque dentro da pasta.
- Há a necessidade de acrescentar países se for necessário.
