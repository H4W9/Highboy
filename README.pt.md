<p align="center">
  <img src="pics/Highboy_repo.png" alt="HighBoy Banner" width="1000"/>
</p>


# Firmware High Boy (Beta)

[![License](https://img.shields.io/github/license/HighCodeh/TentacleOS)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/HighCodeh/TentacleOS)](https://github.com/HighCodeh/TentacleOS/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/HighCodeh/TentacleOS)](https://github.com/HighCodeh/TentacleOS/network/members)
[![Pull Requests](https://img.shields.io/github/issues-pr/HighCodeh/TentacleOS)](https://github.com/HighCodeh/TentacleOS/pulls)

> **Idiomas**:  [🇺🇸 English](README.md) | [🇧🇷 Português](README.pt.md) 

Este repositório contém um **firmware em desenvolvimento** para a plataforma **High Boy**.  
**Atenção:** este firmware está em **fase beta** e **ainda está incompleto**.

---

## Alvos Oficialmente Suportados

Estamos expandindo o suporte para os chips mais recentes da Espressif:

| Alvo | Status |
| :--- | :--- |
| **ESP32-P4** | Desenvolvimento Principal |
| **ESP32-C5** | Desenvolvimento Principal |

---

## Estrutura do Firmware

Diferente de exemplos básicos com um único `main.c`, este projeto utiliza uma estrutura modular organizada em **components**, que se dividem da seguinte forma:

- **Drivers** – Lida com drivers e interfaces de hardware.  
- **Services** – Implementa funcionalidades de suporte e lógica auxiliar.  
- **Core** – Contém a lógica central do sistema e gerenciadores principais.  
- **Applications** – Aplicações específicas que utilizam os módulos anteriores.

Essa divisão facilita a escalabilidade, reutilização de código e organização do firmware.

Veja a arquitetura geral do projeto:  
<p align="center">
  <img src="pics/architecture.png" alt="Arquitetura do Firmware" width="40%"/>
</p>


## Como utilizar este projeto

Recomendamos que este projeto sirva como base para projetos personalizados com ESP32-S3.  
Para começar um novo projeto com ESP-IDF, siga o guia oficial:  
[Documentação ESP-IDF - Criar novo projeto](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)

### Estrutura inicial do projeto

Apesar da estrutura modular, o projeto ainda mantém uma organização compatível com o sistema de build do ESP-IDF (CMake).

Exemplo de layout:

```bash
├── CMakeLists.txt
├── components
│   ├── Drivers
│   ├── Services
│   ├── Core
│   └── Applications
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md
```

## Simulador HLE nativo

O alvo de emulação de alto nível (HLE) executa a interface do P4, o LVGL, o
armazenamento no host e uma ponte SPI simulada para o C5 no Linux. Ele permite
desenvolver a interface e os fluxos do firmware sem conectar um High Boy.

Instale um compilador C/C++, o CMake e o pacote de desenvolvimento do SDL2:

```bash
cmake -S tools/hle -B build
cmake --build build --target hle_interactive -j
./build/hle_interactive
```

Controles:

| Entrada do High Boy | Teclado |
| :--- | :--- |
| Botões direcionais | Setas ou W/A/S/D |
| OK | Enter, Enter do teclado numérico ou Espaço |
| Voltar | Backspace ou Escape |
| Sair do simulador | Ctrl+Q ou fechar a janela |

Por padrão, o simulador armazena os dados de `/sdcard` em `/tmp/hle_storage`.
Use `HLE_STORAGE_PATH` para escolher outro local:

```bash
HLE_STORAGE_PATH="$PWD/.hle-storage" ./build/hle_interactive
```

Para gerar capturas determinísticas da interface sem abrir uma janela:

```bash
SDL_VIDEODRIVER=dummy \
HLE_SNAPSHOT_PATH=/tmp/high-boy.ppm \
HLE_SNAPSHOT_MS=6500 \
./build/hle_interactive
```

Execute os testes nativos com:

```bash
cmake --build build --target hle_tests -j
ctest --test-dir build --output-on-failure
```

O HLE cobre a interface e os fluxos emulados do firmware. Wi-Fi, Bluetooth,
rádio e outros comportamentos de hardware físico ainda exigem testes no
dispositivo.


---

## Como Contribuir

Contribuições são o que fazem a comunidade open-source um lugar incrível para aprender, inspirar e criar. Qualquer contribuição que você fizer é **muito apreciada**.

1. Faça um Fork do projeto
2. Crie sua Feature Branch (`git checkout -b feat/AmazingFeature`)
3. Faça o Commit de suas alterações usando **Conventional Commits** (`git commit -m 'feat(scope): add some AmazingFeature'`)
4. Faça o Push para a Branch (`git push origin feat/AmazingFeature`)
5. Abra um Pull Request

Por favor, leia nosso [**CONTRIBUTING.md**](CONTRIBUTING.md) para mais detalhes sobre o estilo de codificação e processo de build.

---

## Código de Conduta

Estamos comprometidos em oferecer um ambiente amigável, seguro e acolhedor para todos. Por favor, leia nosso [**Código de Conduta**](CODE_OF_CONDUCT.md) para entender as expectativas ao participar deste projeto.

---

## Nossos Apoiadores

Agradecemos especialmente aos parceiros que apoiam este projeto:

[![PCBWay](pics/PCBway.png)](https://www.pcbway.com)

---

## Licença
Este projeto está licenciado sob a **GNU General Public License v3.0**. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
