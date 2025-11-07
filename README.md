## 👻 Process Doppelganging Exploit 🛡️

## 🕵️ Visão Geral

Este projeto é uma implementação da técnica de evasão conhecida como Process Doppelganging. Esta técnica permite a execução de um binário malicioso (payload) sob o disfarce de um processo legítimo ou de um arquivo inexistente no disco, dificultando a detecção (🚫) por soluções de segurança tradicionais.

O método utiliza a API de Transações do Windows (NTFS Transactions) (🔄) para criar um processo a partir de um arquivo temporariamente escrito e imediatamente desfeito (rollback), garantindo que o executável original nunca seja persistido no sistema de arquivos.

---

## 🛠️ Como Funciona (Resumo Técnico)

A técnica Process Doppelganging envolve quatro passos principais implementados nas funções CreateSectionFromTransaction e CreateProcessFromSection:

---

## 📝 Transação e Escrita (Writter):

É criada uma Transação NTFS para a pasta onde o executável temporário será escrito (transactFile).

O conteúdo do seu payload é escrito neste arquivo transacionado.

---

## 🧱 Criação da Seção (Section Creation):

A função NtCreateSection é chamada, criando um objeto de Seção (Section Object) baseado no arquivo transacionado contendo o payload.

Imediatamente após a criação da seção, a transação é desfeita (RollbackTransaction). Isso apaga o arquivo do disco antes que o processo seja criado a partir dele, eliminando a evidência no sistema de arquivos.

---

## 🎭 Doppelganging do Processo (Process Doppelganging):

A função NtCreateProcessEx é usada para criar um novo processo em estado suspenso (PS_INHERIT_HANDLES) a partir do objeto de Seção na memória (que contém o payload).

A estrutura de Process Parameters é modificada para apontar para um arquivo "capa" (coverFile, e.g., C:\error.txt) em vez do arquivo executado real, mascarando o verdadeiro binário.

---

## ▶️ Execução:

Um thread remoto é criado no ponto de entrada (entryPoint) do payload, iniciando a execução do código malicioso.

---

## 🚀 Uso

✅ Pré-requisitos

Um compilador C++ (como Visual Studio) (💻) com suporte a APIs do Windows.

Bibliotecas do Windows SDK, incluindo KtmW32.lib (📦).

⚙️ Compilação

Este é um projeto Windows C++ que requer a inclusão dos cabeçalhos nt_init_func.hpp e DoppelProcess.hpp para as funções e estruturas NT/Ntdll.

🏃 Execução

O programa requer um argumento: o caminho para o executável que você deseja injetar (o payload) (📁).

ProcessDoppelganging.exe <Caminho_Para_Seu_Payload.exe>


O arquivo temporário transacionado será criado em transactFile (e.g., C:\Users\CyberClient\Desktop\logger.txt) e o processo irá se mascarar como coverFile (e.g., C:\error.txt).
