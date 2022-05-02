from django.shortcuts import render
from django.http import HttpResponse
import pam
import os

def index(request):
    context = {
        'deposits': None,
        'credits': None,
        'deptitle': 'To see your deposits and credits please log in.',
        'cretitle': '',
        'user': '',
    }

    if request.POST:
        login = request.POST.get('login')
        password = request.POST.get('password')


        if login == None or password == None:
            return render(request, "index.html", context)

        pam_auth = pam.pam()
        if pam_auth.authenticate(login, password):

            depfiles = []
            crefiles = []

            for filename in os.listdir("../../officerDir/deposits"):
                if filename.split('_')[0] == login:
                    print("filename =", filename)
                    depfiles.append('\n')
                    depfiles.append(filename)
                    with open(f"../../officerDir/deposits/{filename}", encoding='utf-8') as f:
                        content = f.read()
                        depfiles.append(content)

            for filename in os.listdir("../../officerDir/credits"):
                if filename.split('_')[0] == login:
                    print("filename =", filename)
                    crefiles.append('\n')
                    crefiles.append(filename)
                    with open(f"../../officerDir/credits/{filename}", encoding='utf-8') as f:
                        content = f.read()
                        crefiles.append(content)

            context = {
                'deposits': depfiles,
                'credits': crefiles,
                'deptitle': 'Deposits',
                'cretitle': 'Credits',
                'user': login,
            }

            return render(request, "index.html", context)

    return render(request, "index.html", context)